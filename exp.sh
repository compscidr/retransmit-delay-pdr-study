# Experiment to examine the pdr-delay tradeoff
# Jason Ernst, 2013
# PhD Thesis

FILENAME="random.bin"
FILESIZE=$(stat -c%s "$FILENAME")
#IP="131.104.49.218"	# lab.jasonernst.com
IP="192.168.0.2"	# ad hoc ip

#store everything in a data folder
rm *.txt
rm -rf data
mkdir data

# Loop through repetitions of the same configuration for statistical signifigance (law of large numbers)
# important: must do repetitions first so that in each repetition, each probability is run once. This
# will avoid time-dependent effects disturbing performance of only one probability.
for r in {1..30}
do	
	rm -rf $logname
	
	# Loop through varying probabilities of retransmission
	for p in {0.0,0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9,1.0,1.1}
	do
		logname="p$p"
		logname+="s$FILESIZE"
		logname+=".txt"
		echo Prob: $p Rep: $r
		./client --log $logname --probability $p --seed $r --ip $IP --filename=$FILENAME
		sleep 2
	done
done

mv *.txt data
