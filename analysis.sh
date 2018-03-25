# script for performing data analysis
# Jason Ernst, May 2013 - University of Guelph
# Source: http://blog.thinkoriginally.com/2011/06/14/gnuplot-box-and-whiskers-plot/

# input:
#   $1: column#
#   $2: filename without the extension
#   $3: maximum y-value
#   $4: units
function process_column
{
	col=$1
	name=$2
	filename=$2
	filename+=".csv"
	
	#remove the old analysis file if it exists
	rm -rf filename
	
	count=0;
	for fn in `ls *.txt | sort -V`;
	do
		sort $fn -n -t, -k$col -o $fn		# first we sort the data so we can easily find min, median, max
		awk -F, -v count="$count" -v filename="$filename" -v col="$col" '{ 
			s+= $col; 
			array[NR]=$col 
		} END {
			for(x=1;x<=NR;x++) {
				sumsq+=((array[x]-(s/NR))^2);
			} 
			if(NR % 2) {
				median = array[(NR + 1) / 2];
			} else {
				median = (array[(NR / 2)] + array[(NR / 2) + 1]) / 2.0;
			}
			if(NR % 4)
			{
				first = (array[int(NR / 4)] + array[int(NR / 4) + 1]) / 2.0;
				third = (array[int(NR / 4 * 3)] + array[int(NR / 4*3)+1]) / 2.0;
			} else {
				first = array[NR / 4];
				third = array[NR / 4 * 3];
			}
			#print "sum:",s;
			#print "average:",s/NR;
			#print "first quartile:",first;
			#print "median:",median;
			#print "third quartile:",third;
			#print "low:",array[1];
			#print "high:",array[NR];
			#print "std dev:",sqrt(sumsq/NR);
			#print "samples:",NR; 
			#print count/10 "," array[1] "," first "," median "," third "," array[NR] >> filename;
			printf "%f,%f,%f,%f,%f,%f\n", count / 10, array[1], first, median, third, array[NR] >> filename
		}' $fn
		count=$((count+1));
	done
	#first column is P, so we do count / 10 to get 0.0 0.1 0.2...etc
	
	#graph the results
	export name=$name
	export ymax=$3
	export units=$4
	
	../plot.sh
}

cd data
rm -rf *.csv
process_column "3" "Pdr" "1" "%"
process_column "7" "Delay" "240" "milliseconds"

cd ..
