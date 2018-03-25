#include <iostream>
#include <boost/asio.hpp>

#define SERVER_PORT	9999
#define MAX_SIZE 65535		//updated to be the max size of a udp packet (without some modification that sends size = 0 and handles differently)
using boost::asio::ip::udp;
using namespace std;

int discover_ip(void);

int main(int argc, char * argv[])
{
	char buffer[MAX_SIZE];
	memset(buffer, 0, MAX_SIZE);
	
	boost::asio::io_service io_service;
	udp::socket socket(io_service, udp::endpoint(udp::v6(), SERVER_PORT));
	boost::asio::ip::udp::endpoint sender_endpoint;
	boost::system::error_code ec;
	
	discover_ip();
	
	cout << "Listening on port: " << SERVER_PORT << endl;
	
	while(1)
	{
		int received_bytes = socket.receive_from(boost::asio::buffer(buffer, MAX_SIZE), sender_endpoint);
		cout << "Received " << received_bytes << " from the network" << endl;
		
		//pull of packet # of buffer
		int packetNumber;
		memcpy(&packetNumber, buffer, sizeof(int));
		packetNumber = ntohl(packetNumber);
		
		//pull of size from buffer
		int size;
		memcpy(&size, buffer + sizeof(int), sizeof(int));
		size = ntohl(size);
		
		//adjust for header
		received_bytes = received_bytes - sizeof(int);
		
		//if we don't get the whole packet for some reason, drop it
		if(size != received_bytes)
		{
			cout << "  mismatch with size. expected: " << size << " got: " << received_bytes << endl;
			continue;
		}
		cout << "  #: " << packetNumber << " size: " << size << endl;
		
		//form the ack
		char ack_buffer[2 * sizeof(int)] = {0};
		memset(ack_buffer, 0, 2*sizeof(int));
		memcpy(ack_buffer,buffer,2*sizeof(int));	//send back the packet# and the size received
		
		cout << "  sending back ack: " << packetNumber << endl;
		
		//send back ack
		udp::socket ack_socket(io_service, udp::endpoint(udp::v6(), 0));
		socket.send_to(boost::asio::buffer(ack_buffer,2*sizeof(int)), sender_endpoint);
	}
	
	return 0;
}

/*
 * Determines all of the ip addresses on all of the possible interfaces
 * of this device and outputs them
 */
int discover_ip(void)
{
	int sockfd, num_ifaces, i;
	struct ifconf ifc;
	struct ifreq *ifr;
	struct ifreq * interface;
	char info[1024];
	char ip[INET_ADDRSTRLEN];
	
	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return -1;

	//call the ioctl function to get a list of interfaces + information
	ifc.ifc_len = sizeof(info);
	ifc.ifc_buf = info;
	if(ioctl(sockfd, SIOCGIFCONF, &ifc) <0)
	{
		close(sockfd);
		return -1;
	}

	//iterate through the list of interfaces
	ifr = ifc.ifc_req;
	num_ifaces = ifc.ifc_len / sizeof(struct ifreq);
	for(i = 0; i < num_ifaces; i++)
	{
		interface = &ifr[i];
		inet_ntop(AF_INET, &((struct sockaddr_in *)&interface->ifr_addr)->sin_addr, ip, INET_ADDRSTRLEN);
		cout << "Found interface name: " << interface->ifr_name << " IP: " << ip << endl;
	}

	close(sockfd);
		
	return 0;
}
