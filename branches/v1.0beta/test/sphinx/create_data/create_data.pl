#!/usr/bin/env perl
print "start...\n";

$DIR=$ENV{'PWD'};

open(WORDSFILE,"words.txt") || die "can't open words.txt file:$!";
@words;
$words_num=0;
while(<WORDSFILE>){
  chomp;
  $words[$words_num]=$_;
  $words_num++;
}

close(WORDSFILE);
$doc_num=10000*1;

print $words_num;

srand();

sub random_text {
  ($num)=@_;
  $n=int(rand($num))+1;
  my $text='';
  for(my $i=0;$i<$n;$i++){
    $idx=int(rand($words_num));
    $text=$text.$words[$idx];
  }
  #print $text;
  return $text;
}

print random_text(10);

$MEM_OUT_FILE="/dev/shm/test.mysql.tmp2.txt";
open(OUT,">$MEM_OUT_FILE") || die "can't open out file:$!";
print OUT "set names utf8;\n";
$batch_size=1000;
for (my $t=0;$t<$doc_num;){
  print OUT "insert into documents (id,title,content,date_added,author_id,group_id) values\n";
  for(my $i=0;$i<$batch_size;$i++){
    $title=random_text(10);
    $content=random_text(100);
    $r=int(rand(1000000))+1;
    $date_added=`date -d "-${r}second" +"%Y-%m-%d %H:%M:%S"`;
    chomp($date_added);
    $author_id=int(rand(100000))+1;
    $group_id=int(rand(1000))+1;
    if( $i==0 ) {
      $out='';
    }else{
      $out=',';
    }
    print OUT "$out(null,'$title','$content','$date_added','$author_id','$group_id')\n";
  }
  print OUT ";";
  $t=$t+$batch_size;
  print $t."\n";
}
close(OUT);

$OUT_FILE="$DIR/test.create.data.mysql.2.txt";
system("rm -rf $OUT_FILE");
system("mv $MEM_OUT_FILE $OUT_FILE");

