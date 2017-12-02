#!/bin/bash

#
a=`ps aux | grep dbmilter | grep -v grep | awk '{print $4}'`
d=`date +%s`

echo $d, $a
