sudo apt-get update
sudo apt-get install -y ca-certificates
sudo update-ca-certificates
lcov --version
gcov --version
wget https://github.com/cgreen-devs/cgreen/releases/download/1.6.1/cgreen-1.6.1-x86_64-linux-gnu.deb
sudo dpkg -i ./cgreen-1.6.1-x86_64-linux-gnu.deb
cgreen-runner --version
gem install coveralls-lcov
