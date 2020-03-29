cd ..
unzip ns-3.30.1.zip
rm -rf ns-3.30.1/scratch
cp -r ns-3.30.1/* simulator/
rm -r ns-3.30.1
cd simulator || exit 1
