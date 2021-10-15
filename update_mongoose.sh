echo "If there is an error try again or git reset."
rm src/mongoose.c src/mongoose.h
curl https://raw.githubusercontent.com/cesanta/mongoose/master/mongoose.c -o src/mongoose.c
curl https://raw.githubusercontent.com/cesanta/mongoose/master/mongoose.h -o src/mongoose.h
