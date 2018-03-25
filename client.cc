#include <iostream>
#include <fstream>
#include <time.h>
#include <boost/program_options.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::udp;
namespace po = boost::program_options;
using namespace std;

//todo: include a packet size parameter
//must deal with packets which are broken down further

#define SERVER_PORT	9999
#define MAX_SIZE	30720-4		//works out to roughly 1000kb/sec or 8000kbit/s (8Mbps)

enum {TX_FAIL, TX_SUCCESS};

int bytes_sent = 0;
int bytes_received = 0;
int packets_received = 0;
int packets_sent = 0;
struct timeval start_time;	//recorded when the client starts

void printStats(void);
void send(char * buffer, int size, string IP, double P);
int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y);

int main(int argc, char * argv[])
{
	double probability = 0.0;
	string filename = "";
	string IP = "127.0.0.1";
	string log = "";
	int seed = time(NULL);
	
	// Declare the supported options.
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "produce help message")
		("probability", po::value<double>(), "set probability of retransmission from the client (between 0 and 1, default 0)")
		("ip", po::value<string>(), "ipv4 address of server to send the data to (required)")
		("log", po::value<string>(), "log file name")
		("seed", po::value<int>(), "set the seed (experiment rep) for reproducible results")
		("filename", po::value<string>(), "file to send to server");
	po::variables_map vm;
	
	// try to convert the options to their appropriate types (exception thrown otherwise)
	try
	{
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);    
	}
	catch(std::exception const&  ex)
	{
		cout << desc << "\n";
		return 1;
	}
	
	//display help message
	if(vm.count("help")) 
	{
		cout << desc << "\n";
		return 1;
	}
		
	//set the probability, otherwise default to 0.0
	if(vm.count("probability"))
	{
		probability = vm["probability"].as<double>();
		if((probability < 0) || (probability > 1))
		{
			cout << desc << "\n";
			return 1;
		}
	}
	if(vm.count("ip"))
		IP = vm["ip"].as<string>();
	else
	{
		cout << desc << "\n";
		return 1;
	}
	if(vm.count("log"))
		log = vm["log"].as<string>();
	if(vm.count("seed"))
		seed = vm["seed"].as<int>();
	if(vm.count("filename"))
		filename = vm["filename"].as<string>();
	
	ifstream file;
	file.open(filename.c_str(), ios::in|ios::binary);
	
	if(file.is_open())
	{
		//setup the seed
		srand48(seed);
		
		if (gettimeofday(&start_time,NULL))
			throw;
		
		while(file.good())
		{
			char buffer[MAX_SIZE] = {0};
			file.read(buffer, MAX_SIZE);
			cout << "Read " << file.gcount() << " bytes" << endl;
			if(file.gcount()>0)
				send(buffer, file.gcount(), IP, probability);
		}
		
		printStats();
		
		file.close();
	}
	else
	{
		cerr << "Couldn't open file: '" << filename << "' for input " << endl;
		return 1;
	}
	
	//append to the existing log file, all of the stats we collected from this run
	if(log != "")
	{
		ofstream ofs;
		ofs.open (log.c_str(), std::ofstream::out | std::ofstream::app);
		
		struct timeval end_time, difference;
		if (gettimeofday(&end_time,NULL))
			throw;
		timeval_subtract(&difference, &end_time, &start_time);
		long long t = difference.tv_sec * (1.0 * pow(10,9)) + difference.tv_usec;
		
		double seconds = (double)t / (double)(1.0 * pow(10,9));
		double milliseconds = (double)t / (double)(1.0 * pow(10,6));
		double goodput = (double)bytes_received / seconds;
		//double avg_delay = (double)seconds / (double)packets_received;
		double avg_delay = (double)milliseconds / (double)packets_sent;			//used received before
		double pdr = (double)packets_received / (double)packets_sent;
		
		ofs << fixed;	//change so non-scientific notation
		ofs << packets_sent  << ",";
		ofs << packets_received << ",";
		ofs << pdr << ",";
		ofs << bytes_sent << ",";
		ofs << bytes_received << ",";
		ofs << seconds << ",";
		ofs << avg_delay << ",";
		ofs << goodput << endl;
		
		ofs.close();
	}
	
	return 0;
}

/*
 * Returns true if the socket is ready to read before
 * the timeout specified in nanoseconds
 */
bool waitAvailable(boost::asio::ip::udp::socket &socket, long timeout)
{
	//setup timeval for timeout
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = timeout;

    //setup rset for timeout
    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(socket.native(), &rset);

    return ::select(socket.native()+1, &rset, NULL, NULL, &tv) > 0;
}

/*
 * Sends size bytes in packets to the ip specified
 * format: <int packet#> <int size> <size bytes of data>
 */
void send(char * buffer, int size, string IP, double P)
{
	cout << "Sending " << size << " bytes with id: " << packets_sent << "..." << endl;
	
	bytes_sent+=size;
	
	char send_buffer[2 * sizeof(int) + size];
	memset(send_buffer, 0, 2 * sizeof(int) + size);
	
	//copy packet # to buffer
	int netlastSent = htonl(packets_sent);
	memcpy(&send_buffer[0], &netlastSent ,sizeof(int));
	
	//copy size to start of buffer
	int netsize = htonl(size);
	memcpy(&send_buffer[sizeof(int)], &netsize, sizeof(int));
	
	//copy data to buffer
	memcpy(&send_buffer[2 * sizeof(int)], buffer, size);

	//send on network
	boost::asio::io_service io_service;
	udp::socket socket(io_service, udp::endpoint(udp::v6(), 0));
	boost::asio::ip::udp::endpoint destination(boost::asio::ip::address::from_string(IP), SERVER_PORT);	
	
	bool state = TX_FAIL;
	do
	{
		try
		{
			int sent_bytes = socket.send_to(boost::asio::buffer(send_buffer, sizeof(int)+size), destination);
			cout << "Sent packet: " << packets_sent << " with " << sent_bytes << " bytes on the network" << endl;
		}
		catch(std::exception &e)
		{
			cout << "Error sending packet" << endl;
		}
		
		//prepare for ack
		char ack_buffer[MAX_SIZE] = {0};
		memset(ack_buffer, 0, MAX_SIZE);
		boost::asio::ip::udp::endpoint sender_endpoint;
		boost::system::error_code ec;
		
		if(!waitAvailable(socket, 5000))
		{
			//decide if we should retransmit or drop here
			//if we retransmit, continue
			double rand = drand48();
			cout << "RAND: " << rand;
			if(rand < P)
			{
				cout << " RETRANS" << endl;
				continue;
			}
			
			//if not increase lastSent
			cout << " DROP" << endl;
			break;
		}
		
		//receive ack
		int ack_received_bytes = socket.receive_from(boost::asio::buffer(ack_buffer, MAX_SIZE), sender_endpoint);
		int current_ack;
		memcpy(&current_ack, ack_buffer, sizeof(int));
		current_ack = ntohl(current_ack);
		int received_bytes;
		memcpy(&received_bytes, ack_buffer+sizeof(int), sizeof(int));
		received_bytes = ntohl(received_bytes);
		cout << "ACK: " << current_ack << " bytes: " << received_bytes << endl;
		
		//success
		if((current_ack == packets_sent) && (size == received_bytes))
		{
			packets_received++;
			state = TX_SUCCESS;
			bytes_received+=size;
		}
		//some problem with either the ACK # or the size
		else
		{
			cout << "  incorrect ACK" << endl;
			//decide if we should retransmit or drop here
			//if we retransmit, continue
			double rand = drand48();
			cout << "RAND: " << rand;
			if(rand < P)
			{
				cout << " RETRANS" << endl;
				continue;
			}
			
			//if not increase lastSent
			cout << " DROP" << endl;
			break;
		}
		
	} while(state != TX_SUCCESS);
	
	
	packets_sent++; //send the next packet
}

int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y)
{/* Perform the carry for the later subtraction by updating y. */
	if (x->tv_usec < y->tv_usec) 
	{
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > 1000000) 
	{
		int nsec = (x->tv_usec - y->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}
     
	/* Compute the time remaining to wait.
	tv_usec is certainly positive. */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;
     
	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec;
}

void printStats(void)
{
	struct timeval end_time, difference;
	if (gettimeofday(&end_time,NULL))
        throw;
	timeval_subtract(&difference, &end_time, &start_time);
	long long t = difference.tv_sec * (1.0 * pow(10,9)) + difference.tv_usec;
	
	double seconds = (double)t / (1.0 * pow(10,9));
	double goodput = (double)bytes_received / seconds;
	double pdr = (double)packets_received / (double)packets_sent;
	
	cout << "Packets Sent: " << packets_sent << " Packets Received: " << packets_received << " PDR: " << pdr << " Bytes Sent: " << bytes_sent << " Bytes Received Successfully: " << bytes_received << " Total time: " << seconds << "s Delay: " << (double)seconds / packets_received << "s. Goodput:" << goodput << "bytes/s" << endl;
}
