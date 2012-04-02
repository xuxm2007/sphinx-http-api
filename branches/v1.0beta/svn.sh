svn export \
    https://hontlong.googlecode.com/svn/trunk/sphinx_http_api\
    sphinx-http-api \
    --username hontlong@gmail.com --password w7z7r9y5

svn import \
    sphinx-http-api \
    https://sphinx-http-api.googlecode.com/svn/trunk/ \
    --username hontlong@gmail.com --password w7z7r9y5 -m "inital"

rm -rf ./sphinx-http-api

svn checkout \
    https://sphinx-http-api.googlecode.com/svn/trunk/ sphinx-http-api \
    --username hontlong@gmail.com --password w7z7r9y5
