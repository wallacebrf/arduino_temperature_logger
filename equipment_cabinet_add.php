<?php
//requires library: https://github.com/influxdata/influxdb-client-php#install-the-library

//***************************************
//USER VARIABLES
//***************************************
$measurement="House_Temp_Hum";
$config_file_location="/volume1/web/config/config_files/config_files_local/house_config.txt";
//***************************************
//START OF CODE
//***************************************
error_reporting(E_NOTICE);
$data = file_get_contents("".$config_file_location."");
$pieces = explode(",", $data);
$influxdb_host=$pieces[12];
$influxdb_port=$pieces[13];
$influxdb_name=$pieces[14];
$influxdb_pass=$pieces[16];
$script_enable=$pieces[17];
$sensor_name="Garage_Outside";
include $_SERVER['DOCUMENT_ROOT']."/functions.php";
require $_SERVER['DOCUMENT_ROOT']."/admin/vendor/autoload.php";

use InfluxDB2\Client;
use InfluxDB2\Point;

$generic_error="";

if ($script_enable==1){
	
	
	if ($_GET['middle_sensor']==4096.0 || $_GET['left_sensor']==4096.0 || $_GET['right_sensor']==4096.0){
		include $_SERVER['DOCUMENT_ROOT']."/admin/garage_mail.php";
	}

	[$sensor1_influx, $generic_error] = test_input_processing($_GET['middle_sensor'], 0.0, "float", -100.0, 120.0);
	[$sensor2_influx, $generic_error] = test_input_processing($_GET['left_sensor'], 0.0, "float", -100.0, 120.0);
	[$sensor3_influx, $generic_error] = test_input_processing($_GET['right_sensor'], 0.0, "float", -100.0, 120.0);
	$sensor_average_influx=($sensor1_influx+$sensor2_influx+$sensor3_influx)/3.0;
	
	$post_url="".$measurement.",sensor_name=$sensor_name sensor1=$sensor1_influx,sensor2=$sensor2_influx,sensor3=$sensor3_influx,average=$sensor_average_influx";

$client = new InfluxDB2\Client(["url" => "http://".$influxdb_host.":".$influxdb_port."", "token" => "".$influxdb_pass."",
    "bucket" => "".$influxdb_name."",
    "org" => "home",
    "precision" => InfluxDB2\Model\WritePrecision::NS
]);
$write_api = $client->createWriteApi();
$write_api->write($post_url);
$write_api->close();
$client->close();

	print $sensor_average_influx;
}
?>
