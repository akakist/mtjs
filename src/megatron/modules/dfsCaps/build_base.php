##!/usr/bin/php
<?php
$db = mysql_connect('localhost:/tmp/mysqld.sock', 'root','');
#network,geoname_id,registered_country_geoname_id,represented_country_geoname_id,is_anonymous_proxy,is_satellite_provider,postal_code,latitude,longitude,accuracy_radius

die;
mysql_query("create table if not exists city_block4
(
    network varchar(100),
    geoname_id int,
    registered_country_geoname_id int,
    represented_country_geoname_id int,
    is_anonymous_proxy int,
    is_satellite_provider int,
    postal_code varchar(100),
    latitude double,
    longitude double,
    accuracy_radius double
)
") or die(mysql_error());
?>