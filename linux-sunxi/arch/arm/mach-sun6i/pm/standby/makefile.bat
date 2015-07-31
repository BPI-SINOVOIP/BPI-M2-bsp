@echo 
@echo on
copy .\resume0.bin .\super\resume\resume0.code
gen_check_code.exe .\super\resume\resume0.code .\super\resume\resume0.code.tmp
copy .\super\resume\resume0.code.tmp .\super\resume\resume0.code
echo done!
pause
@echo Finished!
	