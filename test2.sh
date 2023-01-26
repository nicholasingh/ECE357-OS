#Change the line below to that it points to your shell
#!./msh
#/tmp always exists on UNIX systems and is 777, so this will work
cd /tmp
pwd
#this should take you back to your homedir
cd
pwd

cd projects

echo TESTINGTESTING123 >testfile3.out
#The following is a command that presumably doesn't exist.  This error
#should not cause the entire script to end
lssssssss 
chmod 000 testfile3.out
#The I/O redirection should fail on the following
echo BLAH >>testfile3.out
#After you invoke this script from the "real" shell, echo $?
#it should reflect the failure of the above I/O redirection which per
#the problem set spec sheet will be a 1 exit status
