<?php
	parse_str($_SERVER['QUERY_STRING'], $destination);
	echo $_SERVER['QUERY_STRING'];
	echo $_SERVER["REMOTE_ADDR"];
	echo $destination;
	echo "This is test PHP script \n";
	return "PHP IS COOL";

?>