fastchess.exe ^
-engine cmd=x64/Release/Eia-v0.5.exe name=New ^
-engine cmd=x64/Release/Eia-v0.5_stable.exe name=Stable ^
-each tc=5+0.05 -rounds 50 -repeat ^
-resign movecount=3 score=800 ^
-ratinginterval 5 ^
-openings file=C:\neurocore\chess\datasets\top200.pgn format=pgn order=random ^
-pgnout notation=san file=C:\neurocore\chess\games\test100.pgn
