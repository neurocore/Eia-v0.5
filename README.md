# Eia v0.5

A UCI chess engine written in C++20. It uses a classic alpha‑beta search with a hand‑crafted evaluation function optimised via Texel tuning (AdaGrad).

## Features

- **Black Magic Bitboards** – the idea of Volker Annuss to slightly decrease tables size.
- **Hand‑crafted evaluation** – tuned with Texel's method using AdaGrad (more details below).
- **Search** – Principal Variation Search (PVS) with LMR and quiescence search.

## Strength

Rated approximately **2200–2300 Elo** in blitz (based on local testing with similar engines).

## Release

In current state isn't released yet. Please wait a little while.

## Usage

The engine is a console application. For comfortable play or analysis, run it under a UCI‑compatible graphical interface (GUI). Popular options include:

- [Arena](http://www.playwitharena.de/)
- [ChessBase (Fritz, etc.)](https://en.chessbase.com/)
- [Scid vs. PC](https://scidvspc.sourceforge.net/)
- [Lucas Chess](https://lucaschess.pythonanywhere.com/)

## Evaluation Tuning

The evaluation parameters were optimised using the Texel's tuning method (minimising mean squared error of game outcome prediction) with AdaGrad. Training was performed on a dataset made for engine [Ethereal](https://github.com/AndyGrant/Ethereal) by Andrew Grant. In his repository, you may find Tuning.pdf paper, which is very useful when implementing the method. In my implementation i omitted all the non‑linear tuning params of positions to make the learning process as simple as possible. Despite the fact that such complex factors such as king safety remained unchanged, it still increased the strength of the game by ~150 Elo in very‑fast time controls (20s+.2s).

## Limitations

- No built‑in opening book.
- No endgame tablebase support (Nalimov, Syzygy, etc.).
- Single‑threaded only.
