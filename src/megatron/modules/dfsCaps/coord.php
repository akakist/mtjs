<?
function mysql_select1row($sql)	
{
	$r=mysql_query($sql) or die(mysql_error().$sql);
	if(!$r){
		echo (mysql_error() . "SQL: $sql");
		return array();
	}
	$res = mysql_fetch_array($r);
	return $res;
}

function dist($lat1,$lon1,$lat2,$lon2)
{
    
    return acos(cos($lat1) * cos($lat2) * cos($lon2-$lon1) + sin($lat1) * sin($lat2))*6372795;
    
}
$db = mysql_connect('localhost:/tmp/mysql.sock', 'root','');
mysql_select_db('coord') or die ('Can"t select database : ' . mysql_error());

//reper1(std::make_pair(0,0)),reper2(std::make_pair(180,0)),reper3(std::make_pair(0,90))
$r1lat=0;
$r1lon=0;
$r2lat=180;
$r2lon=0;
$r3lat=0;
$r3lon=90;

/*
for($i=0;$i<1000000;$i++)
{
    $lat=(rand()%36000)/100;
    $lon=(rand()%36000)/100;
    $r1=dist($lat,$lon,$r1lat,$r1lon);
    $r2=dist($lat,$lon,$r2lat,$r2lon);
    $r3=dist($lat,$lon,$r3lat,$r3lon);
    mysql_query("insert delayed into coord (lat,lon,r1,r2,r3) values ('$lat','$lon','$r1','$r2','$r3')") or die(mysql_error());
    mysql_query("insert into coord (lat,lon,r1,r2,r3) values ('$lat','$lon','$d1','$d2','$d2')") or die (mysql_error());
}
*/
$D=10000;
for($i=0;$i<1000;$i++)
{
    list($lat,$lon,$r1,$r2,$r3)=mysql_select1row("select lat,lon,r1,r2,r3 from coord where id=$i");
    $r=mysql_query("select id,lat,lon from coord where r1>$r1-$D and r1<$r1+$D and r2>$r2-$D and r2<$r2+$D and r3>$r3-$D and r3<$r3+$D") or die(mysql_error());
    while(list($id,$zlat,$zlon)=mysql_fetch_array($r))
    {
//	if(dist($lat,$lon,$zlat,$zlon)<$D)
	{
//	    echo $id." OK\n";
	}
//	else 
	{
//	    echo $id." NOK\n";
	}
	
//	echo $id .' ';
    }
    
//    echo "\n";
}
?>
