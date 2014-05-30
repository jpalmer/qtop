MINE=`qtop --brief`
FREE=`qtop --free complex`
if [ -z $MINE ]
then
    MINE=0;
fi
echo "^fg(green)Free:^fg(gray)$FREE ^fg(green)Mine:^fg(gray)$MINE"
