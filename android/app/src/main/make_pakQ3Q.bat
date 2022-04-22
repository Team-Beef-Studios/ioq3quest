cd assets
del pakQ3Q.pk3
cd ..
powershell Compress-Archive pakQ3Q/* pakQ3Q.zip
rename pakQ3Q.zip pakQ3Q.pk3
move pakQ3Q.pk3 assets/