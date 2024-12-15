# 0- Setup

1- Reiniciar as maquinas: systemctl restart networking

# 1- Configure IP Network

tux2 ligar eth1 a Switch2
No tux2: ifconfig eth1 172.16.121.1/24

tux 3 ligar eth1 a Switch3
No tux3: ifconfig eth1 172.16.120.1/24

tux 4 ligar eth1 a Switch4
No tux4: ifconfig eth1 172.16.120.254/24

# 2- Implement 2 bridges in a switch

tux4 ligar s0 a cisco1
Abrir GKTerm e mudar port para 115200
> /system reset-configuration
> login: admin
> password: 

Criar bridges 
>/interface bridge add name=bridge120
>/interface bridge add name=bridge121

Remover ports da bridge default
> /interface bridge port remove [find interface=ether2]
> /interface bridge port remove [find interface=ether3]
> /interface bridge port remove [find interface=ether4]

Adicionar ports nas bridges
> /interface bridge port add bridge=bridge121 interface=ether2
> /interface bridge port add bridge=bridge120 interface=ether3
> /interface bridge port add bridge=bridge120 interface=ether4

Verificar 
>/interface bridge port print

# 3- Configure a Router in Linux

tux4 ligar eth2 a Switch12
No tux4: ifconfig eth2 172.16.121.253/24

>/interface bridge port remove [find interface=ether12]
>/interface bridge port add bridge=bridge121 interface=ether12


Ligar IP forwarding e desligar ICMP
echo 1 > /proc/sys/net/ipv4/ip_forward
echo 0 > /proc/sys/net/ipv4/icmp_echo_ignore_broadcasts

Verificar MAC e IP adresses
ifconfig eth2

Adicionar Rotas nos outros Tux
No tux2: route add -net 172.16.120.0/24 gw 172.16.121.253
No tux3: route add -net 172.16.121.0/24 gw 172.16.120.254

Verificar rotas
route -n

Testar no tux3:
ping 172.16.120.254
ping 172.16.121.253
ping 172.16.121.1

# 4- Configure a Commercial Router and Implement NAT

router: ligar eth1 ao P3.12; ligar eth2 ao switch11

passar para tux4

>/interface bridge port remove [find interface=ether11]
>/interface bridge port add bridge=bridge121 interface=ether11

mudar consola do switch para consola do router (MTIK)

>/system reset-configuration
> login: admin
> password: 

>/ip address add address=172.16.1.129/24 interface=ether1
>/ip address add address=172.16.121.254/24 interface=ether2

no tux2: route add default gw 172.16.121.254
no tux3: route add default gw 172.16.120.254
no tux4: route add default gw 172.16.121.254