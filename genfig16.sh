#!/usr/bin/env bash
echo "base log"
grep "log " logbaseresult
echo ""
echo "nearpmSD log"
grep "log " lognearpmresult
echo ""
echo "nearpmsync log"
grep "log " lognearpmsyncresult
echo ""
echo "nearpmMD log"
grep "log " lognearpmresult1
echo ""
echo ""
echo "base cp"
grep "cp " cpbaseresult
echo ""
echo "nearpmSD cp"
grep "cp " cpnearpmresult
echo ""
echo "nearpmsync cp"
grep "cp " cpnearpmsyncresult
echo ""
echo "nearpmMD cp"
grep "cp " cpnearpmresult1
echo ""
echo ""
echo "base shadow"
grep "shadow " shadowbaseresult
echo ""
echo "nearpmSD shadow"
grep "shadow " shadownearpmresult
echo ""
echo "nearpmsync shadow"
grep "shadow " shadownearpmsyncresult
echo ""
echo "nearpmMD shadow"
grep "shadow " shadownearpmresult1

