#!/usr/bin/env bash
echo "base log"
grep "log " logbaseresult
echo ""
echo "nearpm log"
grep "log " lognearpmresult
echo ""
echo ""
echo "base cp"
grep "cp " cpbaseresult
echo ""
echo "nearpm cp"
grep "cp " cpnearpmresult
echo ""
echo ""
echo "base shadow"
grep "shadow " shadowbaseresult
echo ""
echo "nearpm shadow"
grep "shadow " shadownearpmresult
cp lognearpmresult lognearpmresult1
cp cpnearpmresult cpnearpmresult1
cp shadowbaseresult shadowbaseresult1
