sudo apt-get update
sudo apt-get install -y ca-certificates
sudo update-ca-certificates
lcov --version
gcov --version
wget https://github.com/cgreen-devs/cgreen/releases/download/1.6.1/cgreen-1.6.1-amd64.deb
sudo dpkg -i ./cgreen-1.6.1-amd64.deb
cgreen-runner --version
gem install coveralls-lcov
