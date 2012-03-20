# /usr/include/x86_64-linux-gnu/sys/queue.h
#sed -e :a -e '$!N;s:\(.*\)\n[ \t]*\({.*\):\1 \2:;ta' -e 'P;D' src/main.cc > main.cc
#sed -e 's/[ \t]\+$//' src/main.cc > main.cc
#sed -e :a -e '$!N;s:\(.*}\)\n[ \t]*\(else.*\):\1 \2:;ta' -e 'P;D' src/main.cc > main.cc

#sed -e "s:( \+:(:g"
#sed -e "s: \+):):g"
sed -e "s:\(\S\)\>\(=\+\|+\|-\|<\+\|>\+\|>=\|<=\|!=\)\<\(\S\):\1 \2 \3:g" 
1,$s:;\(\S\):; \1:g
1,$s:\(for\|if\|while\)(:\1 (:
1,$s:\(\S\){$:\1 {:
1,$s://\(\S\):// \1:
