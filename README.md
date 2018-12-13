# hw3-scheduling-simulation-shaojason999
ucontext refe:  
>[Linux man page]( https://linux.die.net/man/3/makecontext)  
>[我所理解的ucontext族函数](https://www.jianshu.com/p/dfd7ac1402f0)  

Note:
-----------
(1)如果在signal呼叫的function裡再發出signal(e.g. ctrl+z)，不論發出幾次，都會被block住(不回應)，直到這個function離開後才會對這個signal反應，而且只反應一次  
* 特別的: 不要在signal來時呼叫的那個function裡創建一個新的task，不然這個task會無法回應這種signal(會被block住)
(2)
a
