METH=git
METH=https

if [ ! -d letterfunc ]; then
    mkdir letterfunc
    cd letterfunc
    git init
    cd ..
fi
cd letterfunc
git remote add origin $METH://github.com/kuhumcst/letterfunc.git
git pull origin master
cd ..

if [ ! -d parsesgml ]; then
    mkdir parsesgml
    cd parsesgml
    git init
    cd ..
fi
cd parsesgml
git remote add origin $METH://github.com/kuhumcst/parsesgml.git
git pull origin master
cd ..

if [ ! -d rtfreader ]; then
    mkdir rtfreader
    cd rtfreader
    git init
    cd ..
fi
cd rtfreader
git remote add origin $METH://github.com/kuhumcst/rtfreader.git
git pull origin master
cd src
make all
cd ..
cd ..

