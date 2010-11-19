/* main result widget stuff. */
snippetTxtTemplate =
    '<li class="search_snippet">{headHTML}<a href="{url}">{title}</a>{enginesHTML}</h3><div>{summary}</div><div><cite>{cite}</cite><a class="search_cache" href="{cached}">Cached</a><a class="search_cache" href="{archive}">Archive</a><a class="search_cache" href="/search?q={\
enc_query}&amp;page=1&amp;expansion=1&amp;action=similarity&amp;id={id}&amp;engines=">Similar</a></div></li>';

snippetImgTemplate =
    '<li class="search_snippet search_snippet_img"><h3><a href="{url}"><img src="{cached}"><div>{title}{enginesHTML}</div></h3><cite>{cite}</cite><br><a class="search_cache" href="{cached}">Cached</a></li>';

snippetVidTemplate =
    '<li class="search_snippet search_snippet_vid"><a href="{url}"><img class="video_profile" src="{cached}"></a>{headHTML}<a href="{url}">{title}</a>{enginesHTML}</h3><div><cite>{date}</cite></div></li>';

snippetTweetTemplate =
    '<li class="search_snippet"><a href="{cite}"><img class="tweet_profile" src="{cached}" ></a><h3><a href="{url}">{title}</a></h3><div><cite>{cite}</cite><date> ({date})</date><a class="search_cache" href="/search?q={enc_query}&page=1&expansion=1&action=similarity&id={id}\
&engines=twitter,identica">Similar</a></div></li>';

persTemplateFlag = '<img src="@base-url@/plugins/websearch/public/themes/compact/images/perso_star_ico_{prs}.png" style="border: 0;"/>';

var outputDiv = Y.one("#main"), expansionLnk = Y.one("#expansion"), suggDiv = Y.one("#search_sugg"),
pagesDiv = Y.one("#search_page_current"), pagesDivTop = Y.one("#search_page_current_top"), 
persHref = Y.one("#tab-pers"),langInput = Y.one("#tab-language"),
persSpan = Y.one("#tab-pers-flag"), pagePrev = Y.one("#search_page_prev"),
pageNext = Y.one("#search_page_next"), pagePrevTop = Y.one("#search_page_prev_top"), pageNextTop = Y.one("#search_page_next_top");
    
function render_snippet(snippet,pi)
{
    var snippet_html = '';

    // personalization & title head.
    if (snippet.personalized == 'yes')
    {
        snippet.headHTML = '<h3 class=\"personalized_result personalized\" title=\"personalized result\">';
    }
    else
    {
        snippet.headHTML = '<h3>';
    }

    // render url capture.
    if (pi.prs == "on")
    {
        snippet.url = "/qc_redir?q=" + enc_query + "&url=" + encodeURIComponent(snippet.url);
    }

    // render engines. 
    snippet.enginesHTML = '';
    for (j=0, le=snippet.engines.length; j < le; ++j)
    {
        snippet.enginesHTML += '<span class=\"search_engine search_engine_' + snippet.engines[j]  + '\" title=\"' + snippet.engines[j] + '\"><a href=\"';
        snippet.enginesHTML += '@base-url@/search';
        if (ti.img == 1)
            snippet.enginesHTML += '_img';
        snippet.enginesHTML += '?q=' + enc_query;
        snippet.enginesHTML += '&page=1&expansion=1&action=expand&engines=' + snippet.engines[j] + '\">';
        snippet.enginesHTML += '</a></span>';
    }
    
    // encoded query.
    snippet.enc_query = enc_query;

    var shtml = '';
    if (ti.txt == 1)
        shtml = Y.substitute(snippetTxtTemplate, snippet);
    else if (ti.img == 1)
        shtml = Y.substitute(snippetImgTemplate, snippet);
    else if (ti.vid == 1)
        shtml = Y.substitute(snippetVidTemplate, snippet);
    else if (ti.twe == 1)
        shtml = Y.substitute(snippetTweetTemplate, snippet);

    return shtml;
}

function render_snippets(rsnippets,pi)
{
    var snippets_html = '<div id="search_results"><ol>';

    if (pi.prs == "on")
	rsnippets.sort(sort_score);
    else rsnippets.sort(sort_meta);

    var k = 0;
    for (id in rsnippets)
    {
            var snippet = rsnippets[id]
	k++;

        if (k < (pi.cpage-1) * pi.rpp)
	    continue;
        else if (k > pi.cpage * pi.rpp)
	    break;

        snippets_html += render_snippet(snippet,pi);
    }
    return snippets_html + '</ol></div>';
    return snippets_html + '</ol></div>';
}

var isEven = function(someNumber)
{
    return (someNumber%2 == 0) ? true : false;
}

function render_clusters(clusters,labels,pi)
{
    var clusters_html = '<div id="search_results" class="yui3-g clustered">';
    var clusters_c1_html = '<div class="yui3-u-1-2 first">';
    var clusters_c2_html = '<div class="yui3-u-1-2">';
    for (c=0;c<clustered;++c)
    {
        var chtml = '';
        if (isEven(c))
	    clusters_c1_html = render_cluster(clusters[c],labels[c],clusters_c1_html,pi);
	else clusters_c2_html = render_cluster(clusters[c],labels[c],clusters_c2_html,pi);
    }
    return clusters_html + clusters_c1_html + "</div>" + clusters_c2_html + "</div></div></div>";
}

function render_cluster(cluster,label,chtml,pi)
{
    var l = cluster.length;
    if (l == 0)
        return chtml;
    chtml += '<div class="cluster"><h2>' + label + ' <font size="2"> (' + l + ')</font></h2><br><ol>';
    for (i=0;i<l;++i)
    {
        var s = cluster[i];
        var shtml = render_snippet(s,pi);
        chtml += shtml;
    }
    chtml += '</div></ol><div class="clear"></div>';
    return chtml;
}

function render()
{
    var pi;
    if (ti.txt == 1)
	pi = pi_txt;
    else if (ti.img == 1)
        pi = pi_img;
    else if (ti.vid == 1)
	pi = pi_vid;
    else if (ti.twe == 1)
        pi = pi_twe;

    var clusters;
    if (clustered > 0)
    {
        clusters = new Array(clustered);
	for (i=0;i<clustered;++i)
	    clusters[i] = new Array();
    }

    var rsnippets = [];
    var ns = 0;
    for (id in hashSnippets)
    {
        var snippet = hashSnippets[id];

	// check snippet type for rendering.
	if (ti.txt == 1
            && (snippet.type == "image" || snippet.type == "video_thumb" || snippet.type == "tweet"))
	    continue;
	if (ti.img == 1 && snippet.type != "image")
	    continue;
	else if (ti.vid == 1 && snippet.type != "video_thumb")
            continue;
	else if (ti.twe == 1 && snippet.type != "tweet")
            continue;
        if (clustered > 0)
        {
            if ('cluster' in snippet)
            {
                clusters[snippet.cluster].push(snippet);
            }
        }
        else rsnippets.push(snippet);
        ns++;
    }
    
    // render snippets.
    var output_html = '';
    if (clustered > 0)
        output_html = render_clusters(clusters,labels,pi);
    else output_html = render_snippets(rsnippets,pi);
    outputDiv.setContent(output_html);

    // compute page number.
    var p = new rpage();
    p.cpage = pi.cpage;
    var max_page = 1;
    if (ns > 0)
        max_page = ns / pi.rpp;

    pagesDiv.setContent(pi.cpage);
    pagesDivTop.setContent(pi.cpage);
    if (pi.cpage > 1) {
	pagePrev.setStyle('display',"inline");
	pagePrevTop.setStyle('display',"inline");
    }
    else {
	pagePrev.setStyle('display',"none");
	pagePrevTop.setStyle('display',"none");
    }
    if (pi.cpage < max_page) {
	pageNext.setStyle('display',"inline");
	pageNextTop.setStyle('display',"inline");
    }
    else {
	pageNext.setStyle('display',"none");
	pageNextTop.setStyle('display',"inline"); // XXX: "none" leads to bad rendering.
    }
    
    // expansion image.
    expansionLnk.setAttribute('class',"expansion_" + String(pi.expansion));
    expansionLnk.setContent(pi.expansion);

    // personalization.
    var persHTMLf = Y.substitute(persTemplateFlag, pi);
    persSpan.setContent(persHTMLf);

    // render language.
    langInput.set('value',lang);

    // query suggestion.
    suggDiv.setContent(pi.suggestion);
}
