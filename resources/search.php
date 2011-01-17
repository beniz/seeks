<?

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

if($_SERVER['HTTPS']) $scheme = 'https://';
else $scheme= 'http://';

$seeks_uri = 'http://s.s';
$proxy = 'localhost:8250';
$base_script = $_SERVER['SCRIPT_NAME'];
$base_url = $scheme.$_SERVER['HTTP_HOST'].$base_script;

if ($_SERVER['REQUEST_URI'] == '/search_exp.php') { header('Location: '.$base_url.'/websearch-hp'); }
else $url = $seeks_uri.str_replace($base_script, '', $_SERVER['REQUEST_URI']);
$lang_head = $_SERVER['HTTP_ACCEPT_LANGUAGE'];
$referer = $_SERVER['HTTP_REFERER'];

$qc_redir = array();
preg_match('/qc_redir/', $url, $qc_redir);

$tbd = array(); 
preg_match('/tbd/', $url, $tbd);

$curl = curl_init();
curl_setopt($curl, CURLOPT_URL, $url);
if ($qc_redir[0] == "qc_redir"
 || $tbd[0] == "tbd")
{
 curl_setopt($curl, CURLOPT_HEADER, true);
}
curl_setopt($curl, CURLOPT_PROXY, $proxy);
curl_setopt($curl, CURLOPT_RETURNTRANSFER, 1) ;
curl_setopt($curl, CURLOPT_HTTPHEADER, array("Seeks-Remote-Location: ".$base_url,"Accept-Language: ".$lang_head,"Referer: " .$referer));
$result = curl_exec($curl);
$result_info = curl_getinfo($curl);
if(curl_errno($curl)) echo 'CURL ERROR: '.curl_error($curl);
curl_close($curl);

header('Content-Type: '.$result_info['content_type']);

if ($qc_redir[0] == "qc_redir")
    || $tbd[0] == "tbd")
{
 $status_code = array();
 preg_match('/\d\d\d/', $result, $status_code);
 switch( $status_code[0] ) {
  case 302:
  $location = array();
  preg_match('/Location: (.*)/', $result, $location);
  $location[0] = substr($location[0],10);
  header("Location: ". $location[0]);
  break;
  default:
 }
}

echo $result;
?>
