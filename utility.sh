# 
# Works similar to the analysis script, except computes a utility
# based on two columns.
# 
# Several key points:
# - how to normalize columns?
#		- for pdr, already between 0 and 1
#		- for delay, maybe according to category with a threshold for the category
# - weighting between columns in the utility
#
# should generate one utility figure for each probability
#
# input:
#	 $1: data file 1
#	 $2: data file 1 max
#	 $3: data file 2
#	 $4: data file 2 max
#
# if max is one, then already normalized
#
# output: for each line (probability), one file with the following
# weight,median f1,median f2, u(f1), u(f2), utility with weight
function process_utility_vs_prob
{
	rm -rf *.ucsv
	join -1 1 -2 1 -t, -a1 $1 $3 | awk -F, -v max1="$2" -v max2="$4" {'filename=($1 ".ucsv"); for (w = 0; w <= 10; w++){ if(max1==1){v1=$4}else {if($4>max1){$4=max1;}; v1=((max1-$4)/max1);}; if(max2==1){v2=$9;}else{if($9>max2){$9=max2}; v2=$9/max2;}; print w/10 "," $4 "," $9 "," v1 "," v2 "," (v1*w/10) + (v2*(10-w)/10) >> filename };'}
	for fn in `ls *.ucsv | sort -V`;
	do
		export name=$fn
		../plot2.sh
	done
	rm -rf *.ucsv
}

# plots u1 vs u2 (utility tradeoff graph)
# assumes:
# u1 = pdr
# u2 = delay
#
# todo: make more general later
function process_util_vs_util
{
	rm -rf *.ucsv
	# median f1, median f2, u(f1), u(f2)
	# join -1 1 -2 1 -t, -a1 $1 $3 | awk -F, {'if($4 < 50){v1=1}else if($4 > 250){v1=0} else {v1 = (200-$4)/200}; if($9 > 0.75){v2=1;}else if($9 < 0.2) {v2=0;} else {v2=$9}; print $4 "," $9 "," v1 "," v2 >> "out.ucsv";'}
	
	# http://dinodini.wordpress.com/2010/04/05/normalized-tunable-sigmoid-functions/
	join -1 1 -2 1 -t, -a1 $1 $3 | awk -F, {'if($4 < 50){v1=1}else if($4 > 200){v1=0} else {t=1-(($4-50)/(200-50)); v1=t;}; print $4 "," $9 "," v1 "," $9 >> "out.ucsv";'}
	export name=out.ucsv
	../plot3.sh
	#rm -rf *.ucsv
}

./analysis.sh
cd data
#process_utility_vs_prob "Delay.csv" 100 "Pdr.csv" 1		# max delay 50 (VOIP)
process_util_vs_util "Delay.csv" 50 "Pdr.csv" 1		# max delay 50 (VOIP)
cd ..
