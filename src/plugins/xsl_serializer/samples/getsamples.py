#!/usr/bin/python
# -*- coding: utf-8 -*-
# test script for Seeks API 0.4
# Stéphane Bonhomme <stephane.bonhomme@seeks-project.info>

import os
from lxml import etree as ET
import json
import urllib2


SERVER="http://localhost:8080"

#SERVER="http://s.s"
PROXY={}
#PROXY={'http':'http://127.0.0.1:8250'}
FMT="json"
QUERIES=["search engines"]

# get api wiki page, for prototypes 
CMD="""xmllint --html --xmlout --dropdtd  --format http://seeks-project.info/wiki/index.php/API-0.4.0 2>/dev/null | xmlstarlet sel -N h='http://www.w3.org/1999/xhtml' -t -m "//h:p/h:code" -v "normalize-space(.)" -n > samples_index"""
os.popen(CMD)
j=json.JSONDecoder()


requests=[]
current_snippet=""

proxy_handler = urllib2.ProxyHandler(PROXY)
opener=urllib2.build_opener(proxy_handler)
urllib2.install_opener(opener)

# parse alla requests in a list
with open('samples_index') as f:
    for l in f:
        requests.append(l.strip().split(' '))


for q in QUERIES:
    for r in requests:
        try:
            method,path=r
        except:
            continue
        if method != 'GET':
            continue
        path=path.replace("{query}",q)
        path=path.replace("{snippet_id}",current_snippet)
        url=SERVER+urllib2.quote(path)+"?output="+FMT
        print "getting "+url
        
        try:
            res=urllib2.urlopen(url)
        except:
            print "urllib error"
            continue
        if res.getcode() != 200:
            print "-> Erreur %d"%res.getcode()
            continue
        r=res.read()
        rx=None

        if FMT=='xml':
            try:
                rx=ET.XML(r)
            except:
                print "error parsing xml result"
            if rx is not None:
                try:
                    snippets=rx.xpath('//snippet/@id')[0]
                except:
                    pass
                
        if FMT=='json':
            try:
                rx=j.decode(r)
            except:
                print "error parsing json result"
            if rx is not None:
                try:
                    current_snippet=str(rx['snippets'][0]['id'])
                except:
                    pass
        


