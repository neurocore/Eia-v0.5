fastchess.exe ^
-engine cmd=x64/Release/Eia-v0.5_stable.exe name=Eia-v0.5 ^
-engine cmd=C:\neurocore\downloads\Arena\Engines\Liquid_v0_1.exe name=Liquid_v0_1 ^
-engine cmd=C:\neurocore\downloads\Arena\Engines\Monarch(v1.7).exe name=Monarch(v1.7) ^
-each tc=5+0.05 -rounds 20 -repeat ^
-resign movecount=3 score=800 ^
-draw movenumber=34 movecount=12 score=20 ^
-ratinginterval 5 ^
-openings file=C:\neurocore\chess\books\8moves_v3.pgn format=pgn order=random ^
-pgnout notation=san file=C:\neurocore\chess\games\tournament.pgn
