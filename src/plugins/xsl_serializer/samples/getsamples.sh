#!/bin/bash

SERVER="http://localhost:8080"
FMT=xml

# get api wiki page, for prototypes 
xmllint --html --xmlout --dropdtd  --format http://seeks-project.info/wiki/index.php/API-0.4.0 2>/dev/null | xmlstarlet sel -N h='http://www.w3.org/1999/xhtml' -t -m "//h:p/h:code[starts-with(normalize-space(),'GET ')]" -v "substring(normalize-space(.),5)" -n > samples_index

sed -i "s/{query}/search+engines/" samples_index
rm -f get_log.txt

while read u 
do
    if [[ $u == *\{snippet_id\} ]]
    then
	echo "has snipppet $u"
	u=$(echo $u | sed "s/{snippet_id}/$SID/")
    fi
    xmlfile=$(echo $u | sed "s#/#_#g").xml
    echo "getting $SERVER$u"
    echo "$SERVER$u?output=$FMT" >> get_log.txt
    curl $SERVER$u"?output=$FMT" 2> /dev/null > $xmlfile
    s=$(xmlstarlet sel -t -m '//snippet[1]' -o "ok" $xmlfile)
    if [[ $s == ok ]]
    then
	echo "contains snipppet $u"
	SID=$(xmlstarlet sel -t -m '//snippet[1]' -v "@id" $xmlfile)
    fi
    echo '----------------------'
    
done < samples_index
