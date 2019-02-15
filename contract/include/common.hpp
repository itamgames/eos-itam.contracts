#include <string>
#include <vector>
using namespace std;

void split(vector<string>& result, const string& str, const string& delimiter)
{
    string::size_type lastPos = str.find_first_not_of(delimiter, 0);
    string::size_type pos = str.find_first_of(delimiter, lastPos);

    while (string::npos != pos || string::npos != lastPos)
    {
        string temp = str.substr(lastPos, pos - lastPos);

        lastPos = str.find_first_not_of(delimiter, pos);
        pos = str.find_first_of(delimiter, lastPos);

        result.push_back(temp);
    }
}

string replaceAll(const string& base, const string& search, const string& change)
{
    size_t findIndex = -1;
    size_t replaceSize = search.size();
    string result = base;

    while((findIndex = base.find(search, findIndex + 1)) != string::npos)
    {
        result.replace(findIndex, replaceSize, change.c_str());
    }

    return result;
}