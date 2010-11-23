    // page information.
    var query = "fullquery";
    var enc_query = "fullquery";
    var lang = '';
    var clustered = 0;

    var queryInput = Y.one("#search_input");
    var langInput = Y.one("#tab-language");

    function page_info()
    {
	this.rpp = xxrpp;  // results per page.
	this.cpage = 1;  // current page.
	this.engines = '';
	this.expansion = 1;
	this.nexpansion = 2;
	this.prs = "xxpers";
	this.ca = "xxca"; // content analysis.
	this.nclusters = "xxnclust";
	this.suggestion = '';
	this.reset = function() {
            this.cpage = 1;
            this.engines = '';
            this.expansion = 1;
            this.nexpansion = 2;
            this.suggestion = '';
	};
    }
    var pi_txt = new page_info();
    var pi_img = new page_info();
    pi_img.rpp = 60;
    var pi_vid = new page_info();
    pi_vid.rpp = 40;
    var pi_twe = new page_info();

    function type_info()
    {
        this.txt = 1;
        this.img = 0;
        this.vid = 0;
        this.twe = 0;
        this.reset = function() {
            this.txt = 0;
            this.img = 0;
            this.vid = 0;
            this.twe = 0;
        };
    }
    var ti = new type_info();

    function rpage()
    {
        this.prev = '';
	this.next = '';
        this.cpage = 1;
    }

    function sort_meta(s1,s2)
    {
	if (s1.seeks_meta < s2.seeks_meta)
	    return 1;
	else if (s1.seeks_meta > s2.seeks_meta)
	    return -1;
	else {
            if (s1.rank > s2.rank)
		return 1;
            else if (s2.rank < s1.rank)
                return -1;
            else return 0;
        }
    }    

    function sort_score(s1,s2)
    {
        if (s1.seeks_score < s2.seeks_score)
            return 1;
        else if (s1.seeks_score > s2.seeks_score)
            return -1;
	else return sort_meta(s1,s2);
    }
    
    var ref_url = "@base-url@/search?q=fullquery&expansion=1&action=expand&rpp=10&content_analysis=off&prs=on&output=json&callback={callback}";
    var hashSnippets = {};
    var nsnippets = 0;
    var labels;

    function refresh_snippets(response)
    {
        var l = response.snippets.length;
        for (i=0; i < l; ++i)
        {
            var snippet = response.snippets[i];
            if (!hashSnippets[snippet.id])
                nsnippets++;
            hashSnippets[snippet.id] = snippet;
        }
    }

    function refresh_clusters(response,labels)
    {
        var c = response.clusters.length;
        for (i=0; i<c; ++i)
        {
            var cluster = response.clusters[i];
            var l = cluster.snippets.length;
            for (j=0; j<l; ++j)
            {
                var snippet = cluster.snippets[j];
                snippet.cluster = i;
                if (!hashSnippets[snippet.id])
                    nsnippets++;
                hashSnippets[snippet.id] = snippet;
            }
            labels[i] = cluster.label;
        }
    }
    
    // results.
    function refresh(url)
    {
        //outputDiv.setContent("Seeking...");
        var resultsWidget = new Y.JSONPRequest(url, {
            on: {
		success: function (response) {
                    lang = response.lang;
                    query = response.query;
                    //TODO: process query for in-query language commands.
		    var oquery = {pquery:query,nlang:''};
		    query = process_query(oquery);

		    enc_query = ':' + lang + '+' + encodeURIComponent(response.query);

                    if ('clusters'in response)
                    {
			clustered = response.clusters.length;
			labels = new Array(clustered);
			refresh_clusters(response,labels);
		    }
		    else
                    {
			clustered = 0;
			refresh_snippets(response);
                    }

                    // expansion.
                    var pi;
                    if (ti.txt == 1)
			pi = pi_txt;
                    else if (ti.img == 1)
			pi = pi_img;
                    else if (ti.vid == 1)
			pi = pi_vid;
                    else if (ti.twe == 1)
			pi = pi_twe;
                    pi.expansion = response.expansion;
                    pi.nexpansion = eval(response.expansion) + 1;
                    pi.suggestion = response.suggestion;
		    
                    // personalization.
                    pi.prs = response.pers;

                    // engines.
                    pi.engines = response.engines;
		                        
		    // rendering.
                    render();
		},

		failure: function () {
                    outputDiv.setContent(failureTemplate);
		}
            }
        });
	resultsWidget.send();
    }
    
    /* send the request. */
    refresh(ref_url);
    
function process_query(obj)
{
    obj.nlang = '';
    if (obj.pquery[0] == ':')
    {
	obj.nlang = obj.pquery.substr(1,2);
	obj.pquery = obj.pquery.substr(4,obj.pquery.length);
	return obj.pquery;
    }
    else return obj.pquery;
}

    // Click the search button to send the JSONP request
    Y.one("#search_form").on("submit", function (e) {
        var nquery = queryInput.get('value');
        var pi;
        if (ti.txt == 1)
            pi = pi_txt;
        else if (ti.img == 1)
        {
            pi = pi_img;
            eimg = "_img";
        }
        else if (ti.vid == 1)
            pi = pi_vid;
        else if (ti.twe == 1)
            pi = pi_twe;
	
	// detect and process in-query commands.
	var prq = {pquery:nquery,nlang:''};
	process_query(prq);
	if (prq.nlang == '')
	{
	    var ilang = langInput.get('value');
	    if (ilang != lang && ilang != '')
		prq.nlang = ilang;
	    else prq.nlang = lang;
	}
        if (query != prq.pquery || lang != prq.nlang)
        {
	    var eimg = '';
            if (ti.img == 1)
                eimg = "_img";
            var url = '@base-url@/search' + eimg + '?' + Y.QueryString.stringify({"q":queryInput.get('value'),"expansion":1,"lang":prq.nlang,"action":"expand","rpp":pi.rpp,"prs":"on","content_analysis":pi.ca,"ui":"dyn","output":"json"}) + "&engines=" + pi.engines + "&callback={callback}";
            for (id in hashSnippets)
                delete hashSnippets[id];
            pi_txt.reset();
            pi_img.reset();
            pi_vid.reset();
	    pi_twe.reset();
            refresh(url);
        }
	queryInput.set('value',prq.pquery);
	pi.cpage = 1;
        clustered = 0;
        render();
        return false;
    });

    Y.one("#expansion").on("click",function(e) {
	var eimg = '';
        var pi;
	if (ti.txt == 1)
            pi = pi_txt;
	else if(ti.img == 1)
	{
            pi = pi_img;
            eimg = "_img";
	}
        else if(ti.vid == 1)
            pi = pi_vid;
	else if(ti.twe == 1)
	    pi = pi_twe;
	var url = '@base-url@/search' + eimg + '?' + Y.QueryString.stringify({"q":queryInput.get('value'),"expansion":pi.nexpansion,"lang":lang,"action":"expand","rpp":pi.rpp,"prs":pi.prs,"content_analysis":pi.ca,"ui":"dyn","output":"json"}) + "&engines=" + pi.engines + "&callback={callback}";
	refresh(url);
        return false;
    });

    Y.one("#cluster").on("click",function(e) {
	var pi;
	if (ti.txt == 1)
	    pi = pi_txt;
	else if (ti.vid == 1)
	    pi = pi_vid;
	else if (ti.twe == 1)
	    pi = pi_twe;
	else return false;
	var url = '@base-url@/search?' + Y.QueryString.stringify({"q":queryInput.get('value'),"expansion":pi.expansion,"lang":lang,"action":"clusterize","clusters":pi.nclusters,"prs":pi.prs,"content_analysis":pi.ca,"ui":"dyn","output":"json"}) + "&callback={callback}";
        refresh(url);
        return false;
    });

    Y.one("#tab-pers").on("click",function(e) {
        var pi;
        if (ti.txt == 1)
            pi = pi_txt;
        else if(ti.img== 1)
            pi = pi_img;
        else if(ti.vid== 1)
            pi = pi_vid;
        else if(ti.twe== 1)
            pi = pi_twe;
        var nprs = "off";
        if (pi.prs == "off")
            nprs = "on";
        var url = '@base-url@/search?' + Y.QueryString.stringify({"q":queryInput.get('value'),"expansion":pi.expansion,"lang":lang,"action":"expand","rpp":pi.rpp,"prs":nprs,"content_analysis":pi.ca,"ui":"dyn","output":"json"}) + "&engines=" + pi.engines + "&callback={callback}";
        refresh(url);
        return false;
    });

    Y.one("#tab-text").on("click",function(e) {
        ti.reset();
        ti.txt = 1;
        clustered = 0;
        if (pi_txt.engines == '')
        {
            var url = '@base-url@/search?' + Y.QueryString.stringify({"q":queryInput.get('value'),"expansion":pi_txt.expansion,"lang":lang,"action":"expand","rpp":pi_txt.rpp,"prs":pi_txt.pers,"content_analysis":pi_txt.ca,"ui":"dyn","output":"json"}) + "&callback={callback}";
            refresh(url);
        }
        else render();
        return false;
    });

    Y.one("#tab-img").on("click",function(e) {
        ti.reset();
        ti.img = 1;
        if (pi_img.engines == '')
        {
            var url = '@base-url@/search_img?' + Y.QueryString.stringify({"q":queryInput.get('value'),"expansion":1,"lang":lang,"action":"expand","rpp":pi_img.rpp,"prs":pi_img.pers,"content_analysis":pi_img.ca,"ui":"dyn","output":"json"}) + "&callback={callback}";
            refresh(url);
        }
        else render();
        return false;
    });

    Y.one("#tab-vid").on("click",function(e) {
        ti.reset();
        ti.vid = 1;
        clustered = 0;
        if (pi_vid.engines == '')
        {
            var url = '@base-url@/search?' + Y.QueryString.stringify({"q":queryInput.get('value'),"expansion":pi_vid.expansion,"lang":lang,"action":"expand","rpp":pi_vid.rpp,"prs":pi_vid.pers,"content_analysis":pi_vid.ca,"ui":"dyn","output":"json"}) + "&engines=youtube,dailymotion&callback={callback}";
            refresh(url);
        }
        else render();
        return false;
    });

    Y.one("#tab-tweet").on("click",function(e) {
        ti.reset();
        ti.twe = 1;
        clustered = 0;
        if (pi_twe.engines == '')
        {
            var url = '@base-url@/search?' + Y.QueryString.stringify({"q":queryInput.get('value'),"expansion":pi_twe.expansion,"lang":lang,"action":"expand","rpp":40,"prs":pi_twe.pers,"content_analysis":pi_twe.ca,"ui":"dyn","output":"json"}) + "&engines=twitter,identica&callback={callback}";
            refresh(url);
        }
        else render();
        return false;
    });

/* Y.one("#tab-urls").on("click",function(e) {                                                                                                                                                                                                                                     
        var url = '@base-url@/search?' + Y.QueryString.stringify({"q":queryInput.get('value'),"action":"urls","ui":"dyn","output":"json"}) + "&callback={callback}";
        refresh(url);
        return false;
	});                                                                                                                                                                                                                                                                                
   Y.one("#tab-titles").on("click",function(e) {                                                                                                                                                                                                                                      
        var url = '@base-url@/search?' + Y.QueryString.stringify({"q":queryInput.get('value'),"action":"titles","ui":"dyn","output":"json"}) + "&callback={callback}";
        refresh(url);
        return false;
	}); */

    Y.one("#tab-types").on("click",function(e) {
        var url = '@base-url@/search?' + Y.QueryString.stringify({"q":queryInput.get('value'),"action":"types","lang":lang,"ui":"dyn","output":"json"}) + "&callback={callback}";
        refresh(url);
        return false;
    });

    Y.one("#search_page_prev").on("click",function(e) {
        if (ti.txt == 1)
            pi = pi_txt;
        else if(ti.img== 1)
            pi = pi_img;
        else if(ti.vid== 1)
            pi = pi_vid;
        else if(ti.twe== 1)
            pi = pi_twe;
        pi.cpage--;
        render();
    });

    Y.one("#search_page_next").on("click",function(e) {
        if (ti.txt == 1)
            pi = pi_txt;
        else if(ti.img== 1)
            pi = pi_img;
        else if(ti.vid== 1)
            pi = pi_vid;
        else if(ti.twe== 1)
            pi = pi_twe;
        pi.cpage++;
        render();
    });

// Shortcut for search, ctrl+s
YUI().use('event-key', function(Y) {
    var handle = Y.on('key', function(e) {
        e.halt();
        Y.one('#search_input').select();
    }, document, 'press:102+ctrl');

});

// Shortcut for expansion, ctrl+e
YUI().use('event-key', function(Y) {
    var handle = Y.on('key', function(e) {
	e.halt();
	document.location = Y.one('#expansion').getAttribute('href');
    }, document, 'press:101+ctrl');

});

// Shortcut for clustering, ctrl+c                                                                                                                                                                                                                                                     
YUI().use('event-key', function(Y) {
    var handle = Y.on('key', function(e) {
	e.halt();
	document.location = Y.one('#cluster').getAttribute('href');
    }, document, 'press:99+ctrl');
});

// Shortcut for previous page, ctrl+<
YUI().use('event-key', function(Y) {
    var handle = Y.on('key', function(e) {
        e.halt();
        document.location = Y.one('#search_page_prev').getAttribute('href');
    }, document, 'press:37+ctrl');
});

// Shortcut for home page, ctrl+h
YUI().use('event-key', function(Y) {
    var handle = Y.on('key', function(e) {
        e.halt();
        document.location = Y.one('#search_home').getAttribute('href');
    }, document, 'press:104+ctrl');
});
