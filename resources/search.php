<?php

/* Copyright Camille Harang, Pablo Joubert, Emmanuel Benazera

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program. If not, see http://www.fsf.org/licensing/licenses/agpl-3.0.html. */

/*****************************************************************************
 * Important change to php.ini:                                              *
 *****************************************************************************
 * You MUST set always_populate_raw_post_data=1 in your php.ini to get this  *
 * working!). E.g. I had to set this in FPM's seeks.conf:                    *
 * php_admin_value[always_populate_raw_post_data] = 1                        *
 *****************************************************************************
 * My file is located at /etc/php5/fpm/pool.d/seeks.conf.                    *
 *****************************************************************************
 */

// Let's fix all of them
error_reporting(E_ALL | E_STRICT);

// Default is HTTP
$scheme = 'http://';

// Is HTTPS set?
if (isset($_SERVER['HTTPS'])) {
	// Then assume https:// shall be used
	$scheme = 'https://';
}

$seeks_uri = 'http://s.s';
$proxy = 'localhost:8250';

$base_url = $scheme . $_SERVER['HTTP_HOST'] . $_SERVER['SCRIPT_NAME'];

if ($_SERVER['REQUEST_URI'] == '/search.php') {
	header('Location: ' . $base_url . '/websearch-hp');

	// Don't miss exit() as the server redirect doesn't abort the script
	exit();
} else {
	$url = $seeks_uri . str_replace($_SERVER['SCRIPT_NAME'], '', $_SERVER['REQUEST_URI']);
}

// Avoid E_NOTICE by initializing variables and checking on array elements
$lang_head = '';
$referer = '';

// Not all browsers are sending this header
if (isset($_SERVER['HTTP_ACCEPT_LANGUAGE'])) {
	$lang_head = $_SERVER['HTTP_ACCEPT_LANGUAGE'];
}

// Not all browsers are sending this header
if (isset($_SERVER['HTTP_REFERER'])) {
	$referer = $_SERVER['HTTP_REFERER'];
}

$qc_redir = array();
preg_match('/qc_redir/', $url, $qc_redir);

$tbd = array();
preg_match('/tbd/', $url, $tbd);

$bqc = array();
preg_match('/find_bqc/', $url, $bqc);

$curl = curl_init();
curl_setopt($curl, CURLOPT_URL, $url);

if ((isset($qc_redir[0]) && $qc_redir[0] == 'qc_redir') || (isset($tbd[0]) && $tbd[0] == 'tbd')) {
	curl_setopt($curl, CURLOPT_HEADER, true);
}

curl_setopt($curl, CURLOPT_CUSTOMREQUEST, $_SERVER['REQUEST_METHOD']);

if (isset($bqc[0]) && $bqc[0] == 'find_bqc') {
	$postdata = file_get_contents('php://input');

	// For debugging purposes
	//* DEBUG: */ file_put_contents('/tmp/foo', ini_get('always_populate_raw_post_data') . ':' . $postdata);

	curl_setopt($curl, CURLOPT_POST, 1);
	curl_setopt($curl, CURLOPT_POSTFIELDS, $postdata);
}

curl_setopt($curl, CURLOPT_PROXY, $proxy);
curl_setopt($curl, CURLOPT_RETURNTRANSFER, 1);

curl_setopt($curl, CURLOPT_HTTPHEADER, array('X-Seeks-Remote-Location: ' . $base_url, 'Accept-Language: ' . $lang_head, 'Proxy-Connection: Close', 'Expect:', 'Referer: ' . $referer));

$result = curl_exec($curl);
$result_info = curl_getinfo($curl);

// Very noisy in syslog:
//* NOISY-DEBUG: */ syslog(LOG_INFO, 'result_info=' . json_encode($result_info));

if (curl_errno($curl)) {
	echo 'CURL ERROR: '.curl_error($curl);
}

curl_close($curl);

if (headers_sent()) {
	// Headers are already sent, cannot continue, maybe error?
	exit();
}

header('Content-Type: ' . $result_info['content_type']);

if ((isset($qc_redir[0]) && $qc_redir[0] == 'qc_redir') || (isset($tbd[0]) && $tbd[0] == 'tbd')) {
	preg_match('/\d\d\d/', $result, $status_code);

	switch( $status_code[0] ) {
	case 302:
		$location = array();
		preg_match('/Location: (.*)/', $result, $location);
		$location[0] = substr($location[0], 10);
		header('Location: '. $location[0]);

		// Don't miss exit() as the server redirect doesn't abort the script
		exit();

	default:
		syslog(LOG_WARNING, 'Status code ' . $status_code[0] . ' found.');
		break;
	}
}

// Very noisy in syslog:
//* NOISY-DEBUG: */ syslog(LOG_INFO, 'result=' . json_encode($result));
echo $result;

?>
