#!/bin/ksh


echo "Starting acceptance tests..."

./metronome 120 2 4 &
METRONOME_PID=$!

sleep 3


echo "Checking: Metronome settings"

echo /dev/local/metronome
echo ""  


echo "Checking: Metronome Help"

ls -l /dev/local/metronome-help

cat /dev/local/metronome-help
echo ""  


echo "set 100 2 4 > /dev/local/metronome"

echo "set 100 2 4" > /dev/local/metronome

sleep 5

cat /dev/local/metronome
echo ""  


echo "set 200 5 4 > /dev/local/metronome"

echo "set 200 5 4" > /dev/local/metronome

sleep 5

cat /dev/local/metronome
echo ""  


echo "stop > /dev/local/metronome"

echo "stop" > /dev/local/metronome

sleep 1


echo "start > /dev/local/metronome"

echo "start" > /dev/local/metronome

sleep 5

cat /dev/local/metronome
echo ""  


echo "pause 3 > /dev/local/metronome"

echo "pause 3" > /dev/local/metronome

sleep 5


echo "pause 10 > /dev/local/metronome"

echo "pause 10" > /dev/local/metronome
echo ""  


echo "bogus > /dev/local/metronome"

echo "bogus" > /dev/local/metronome
echo ""  


echo "set 120 2 4 > /dev/local/metronome"

echo "set 120 2 4" > /dev/local/metronome

sleep 5

cat /dev/local/metronome
echo ""  


echo " cat /dev/local/metronome-help"

cat /dev/local/metronome-help
echo ""  


echo "Writes-Not-Allowed > /dev/local/metronome-help"

echo "Writes-Not-Allowed" > /dev/local/metronome-help
echo ""  


echo "quit > /dev/local/metronome"

echo "quit" > /dev/local/metronome

sleep 1

echo "Metronome Stopped."