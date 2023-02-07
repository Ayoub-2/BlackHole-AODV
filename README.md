# BlackHole-AODV
BlackHole attack on aodv routing protocole simulated with NS-3 on VANET

## Requirements: 

- NS-3.31 
- Netanim-3.108

## Installation : 
### Setup :
```bash
git clone https://github.com/Ayoub-2/BlackHole-AODV.git
cd BlackHole-AODV 
mv aodv/* ./ns-3.31/src/aodv/model/
mv .* ./ns-3.31/scratch/
```
### Run :
```bash
./waf config 
./waf --run blackhole 
```
