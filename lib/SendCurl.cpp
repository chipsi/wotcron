#include "./SendCurl.h"



SendCurl::SendCurl()
{
    // ctor
}



int writer(char *data, size_t size, size_t nmemb, std::string *buffer)
 {
    int result = 0;  
    if (buffer != NULL)
    {
        buffer->append(data, size * nmemb);  
        result = size * nmemb;
    }

    return result;
}

string SendCurl::SendRequest(const char *url, const char *postdata)
{
    
    CURL *curl;
    CURLcode res;
    string buffer;

    curl = curl_easy_init();
    
    curl_easy_setopt(curl, CURLOPT_URL, url );
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    
    res = curl_easy_perform(curl);
      
    /* Check for errors */ 
    if(res != CURLE_OK)
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));

    /* always cleanup */ 
    curl_easy_cleanup(curl);

    return buffer;
}

string SendCurl::SendWGN(string method, string post)
{
    
    // Zmenim string na char
    string tmp_url = server_wgn + method;
    const char *url = tmp_url.c_str();
    
    // Zmenim string na char
    string tmp_post = api_id + post;
    const char *postdata = tmp_post.c_str();
    
    string SendRequest(const char *url, const char *postdata);

    
    string data; 
    data = this->SendRequest(url, postdata);

    int i;
    i = this->SkontrolujStatus(data);
    cout << "Hodnota i : " << i  << endl;
    return data;

}

string SendCurl::SendWOT(string method, string post)
{
    string data;
    // Treba premenit string na char
    string tmp_url = server_wot+method;
    const char *url = tmp_url.c_str();
    
    string tmp_post = api_id+post;
    const char *postdata = tmp_post.c_str();

    string SendRequest(const char *url, const char *postdata);

    data = this->SendRequest(url, postdata);

    if(!this->SkontrolujStatus(data))
    {
        data = this->SendRequest(url, postdata);
        cout << "Cyklus" << endl;
    }
   
    return data;

}

bool SendCurl::SkontrolujStatus(string data)
{   
    
    string tmp;
    unsigned zaciatok   = data.find("status\":\"");
    unsigned koniec     = data.find("\"", zaciatok+9);
    
    tmp = data.substr(zaciatok+9, koniec - (zaciatok+9));
    
    if(tmp.compare("ok") == 0)
    {
         data.clear();
         return true;
    }
    else
    {
        cout << "Fail status: " << data << endl;
        data.clear(); 
        return false;
    }


   
}


SendCurl::~SendCurl()
{

}



