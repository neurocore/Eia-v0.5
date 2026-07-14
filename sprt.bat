fastchess.exe ^
-engine cmd=x64/Release/Eia-v0.5.exe name=New ^
-engine cmd=x64/Release/Eia-v0.5_stable.exe name=Stable ^
-each tc=5+0.05 -rounds 15000 -repeat ^
-resign movecount=3 score=800 ^
-draw movenumber=34 movecount=12 score=20 ^
-ratinginterval 5 ^
-sprt elo0=0 elo1=5 alpha=0.05 beta=0.05 ^
-openings file=C:\neurocore\chess\books\8moves_v3.pgn format=pgn order=random ^
-pgnout notation=san file=C:\neurocore\chess\games\sprt.pgn
