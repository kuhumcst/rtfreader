METH=git
METH=https

if [ ! -d letterfunc ]; then
    mkdir letterfunc
    cd letterfunc
    git init
    git remote add origin $METH://github.com/kuhumcst/letterfunc.git
    cd ..
fi
cd letterfunc
git pull origin master
cd ..

if [ ! -d parsesgml ]; then
    mkdir parsesgml
    cd parsesgml
    git init
    git remote add origin $METH://github.com/kuhumcst/parsesgml.git
    cd ..
fi
cd parsesgml
git pull origin master
cd ..

if [ ! -d rtfreader ]; then
    mkdir rtfreader
    cd rtfreader
    git init
    git remote add origin $METH://github.com/kuhumcst/rtfreader.git
    cd ..
fi
cd rtfreader
git pull origin master
cd src
make all
cd ..
cd ..

