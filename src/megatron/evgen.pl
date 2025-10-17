#!/usr/bin/perl
@r=split '\s',`find ./  -name eventgen.sh`;
foreach(@r)
{
print $_."\n";
     s/eventgen\.sh//g;
    print $_."\n";
    system ("cd $_;sh ./eventgen.sh");
    
}
