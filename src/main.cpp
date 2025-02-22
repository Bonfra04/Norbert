#include <iostream>

#include "infra/infra.hpp"
#include "encoding/encoding.hpp"
#include "css/css.hpp"

using namespace Norbert;
using namespace Norbert::Infra;
using namespace Norbert::Encoding;

int main()
{
    string s = " \"\\22 6miao\"+22.542E+2";
    CSS::Parser::parseStylesheetContents(s);

    return 0;
}