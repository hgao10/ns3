cd ..
unzip ns-3.30.1.zip
rm -rf ns-3.30.1/scratch
cp -r ns-3.30.1/* simulator/
rm -r ns-3.30.1
cd simulator || exit 1
cp src/basic-sim/model/replace-ipv4-global-routing.cc.txt src/internet/model/ipv4-global-routing.cc
cp src/basic-sim/model/replace-ipv4-global-routing.h.txt src/internet/model/ipv4-global-routing.h
