All the swapfile sizes in the simulation are 3000
tracefiles : 
/u/csc369h/winter/pub/a2-traces/tr-simpleloop.ref
/u/csc369h/winter/pub/a2-traces/tr-blocked.ref
/u/csc369h/winter/pub/a2-traces/tr-matmul.ref
traceprogs/tr-linkedlisttrace.ref

	rand
--------------------------------------------------
tr-simpleloop
--------------------------------------------------
m = 50
	Hit count: 7600
	Miss count: 2968
	Clean evictions: 236
	Dirty evictions: 2682
	Total references : 10568
	Hit rate: 71.9152
	Miss rate: 28.0848

m = 100
	Hit count: 7823
	Miss count: 2745
	Clean evictions: 49
	Dirty evictions: 2596
	Total references : 10568
	Hit rate: 74.0254
	Miss rate: 25.9746

m = 150
	Hit count: 7853
	Miss count: 2715
	Clean evictions: 14
	Dirty evictions: 2551
	Total references : 10568
	Hit rate: 74.3092
	Miss rate: 25.6908

m = 200
	Hit count: 7865
	Miss count: 2703
	Clean evictions: 18
	Dirty evictions: 2485
	Total references : 10568
	Hit rate: 74.4228
	Miss rate: 25.5772

--------------------------------------------------
tr-blocked
--------------------------------------------------
m = 50
	Hit count: 2516731
	Miss count: 8293
	Clean evictions: 6890
	Dirty evictions: 1353
	Total references : 2525024
	Hit rate: 99.6716
	Miss rate: 0.3284

m = 100
	Hit count: 2519796
	Miss count: 5228
	Clean evictions: 3964
	Dirty evictions: 1164
	Total references : 2525024
	Hit rate: 99.7930
	Miss rate: 0.2070

m = 150
	Hit count: 2520591
	Miss count: 4433
	Clean evictions: 3160
	Dirty evictions: 1123
	Total references : 2525024
	Hit rate: 99.8244
	Miss rate: 0.1756

m = 200
	Hit count: 2521210
	Miss count: 3814
	Clean evictions: 2507
	Dirty evictions: 1107
	Total references : 2525024
	Hit rate: 99.8490
	Miss rate: 0.1510

--------------------------------------------------
tr-matmul
--------------------------------------------------
m = 50
	Hit count: 1973240
	Miss count: 994912
	Clean evictions: 956138
	Dirty evictions: 38724
	Total references : 2968152
	Hit rate: 66.4804
	Miss rate: 33.5196

m = 100
	Hit count: 2644847
	Miss count: 323305
	Clean evictions: 315834
	Dirty evictions: 7371
	Total references : 2968152
	Hit rate: 89.1075
	Miss rate: 10.8925

m = 150
	Hit count: 2872249
	Miss count: 95903
	Clean evictions: 93453
	Dirty evictions: 2300
	Total references : 2968152
	Hit rate: 96.7689
	Miss rate: 3.2311

m = 200
	Hit count: 2911955
	Miss count: 56197
	Clean evictions: 54368
	Dirty evictions: 1629
	Total references : 2968152
	Hit rate: 98.1067
	Miss rate: 1.8933

--------------------------------------------------
tr-linkedlisttrace
--------------------------------------------------
m = 50
	Hit count: 15760
	Miss count: 336
	Clean evictions: 149
	Dirty evictions: 137
	Total references : 16096
	Hit rate: 97.9125
	Miss rate: 2.0875

m = 100
	Hit count: 15941
	Miss count: 155
	Clean evictions: 4
	Dirty evictions: 51
	Total references : 16096
	Hit rate: 99.0370
	Miss rate: 0.9630

m = 150
	Hit count: 15963
	Miss count: 133
	Clean evictions: 0
	Dirty evictions: 0
	Total references : 16096
	Hit rate: 99.1737
	Miss rate: 0.8263

m = 200
	Hit count: 15963
	Miss count: 133
	Clean evictions: 0
	Dirty evictions: 0
	Total references : 16096
	Hit rate: 99.1737
	Miss rate: 0.8263

==================================================
	fifo
--------------------------------------------------
tr-simpleloop
--------------------------------------------------
m = 50
	Hit count: 7602
	Miss count: 2966
	Clean evictions: 224
	Dirty evictions: 2692
	Total references : 10568
	Hit rate: 71.9341
	Miss rate: 28.0659

m = 100
	Hit count: 7820
	Miss count: 2748
	Clean evictions: 43
	Dirty evictions: 2605
	Total references : 10568
	Hit rate: 73.9970
	Miss rate: 26.0030

m = 150
	Hit count: 7859
	Miss count: 2709
	Clean evictions: 16
	Dirty evictions: 2543
	Total references : 10568
	Hit rate: 74.3660
	Miss rate: 25.6340

m = 200
	Hit count: 7867
	Miss count: 2701
	Clean evictions: 12
	Dirty evictions: 2489
	Total references : 10568
	Hit rate: 74.4417
	Miss rate: 25.5583

--------------------------------------------------
tr-blocked
--------------------------------------------------
m = 50
	Hit count: 2518574
	Miss count: 6450
	Clean evictions: 5201
	Dirty evictions: 1199
	Total references : 2525024
	Hit rate: 99.7446
	Miss rate: 0.2554

m = 100
	Hit count: 2520689
	Miss count: 4335
	Clean evictions: 3121
	Dirty evictions: 1114
	Total references : 2525024
	Hit rate: 99.8283
	Miss rate: 0.1717

m = 150
	Hit count: 2520785
	Miss count: 4239
	Clean evictions: 2979
	Dirty evictions: 1110
	Total references : 2525024
	Hit rate: 99.8321
	Miss rate: 0.1679

m = 200
	Hit count: 2521847
	Miss count: 3177
	Clean evictions: 1893
	Dirty evictions: 1084
	Total references : 2525024
	Hit rate: 99.8742
	Miss rate: 0.1258

--------------------------------------------------
tr-matmul
--------------------------------------------------
m = 50
	Hit count: 1840885
	Miss count: 1127267
	Clean evictions: 1083469
	Dirty evictions: 43748
	Total references : 2968152
	Hit rate: 62.0213
	Miss rate: 37.9787

m = 100
	Hit count: 1884606
	Miss count: 1083546
	Clean evictions: 1061660
	Dirty evictions: 21786
	Total references : 2968152
	Hit rate: 63.4943
	Miss rate: 36.5057

m = 150
	Hit count: 2933748
	Miss count: 34404
	Clean evictions: 32957
	Dirty evictions: 1297
	Total references : 2968152
	Hit rate: 98.8409
	Miss rate: 1.1591

m = 200
	Hit count: 2934270
	Miss count: 33882
	Clean evictions: 32441
	Dirty evictions: 1241
	Total references : 2968152
	Hit rate: 98.8585
	Miss rate: 1.1415

--------------------------------------------------
tr-linkedlisttrace
--------------------------------------------------
m = 50
	Hit count: 15766
	Miss count: 330
	Clean evictions: 137
	Dirty evictions: 143
	Total references : 16096
	Hit rate: 97.9498
	Miss rate: 2.0502

m = 100
	Hit count: 15934
	Miss count: 162
	Clean evictions: 0
	Dirty evictions: 62
	Total references : 16096
	Hit rate: 98.9935
	Miss rate: 1.0065

m = 150
	Hit count: 15963
	Miss count: 133
	Clean evictions: 0
	Dirty evictions: 0
	Total references : 16096
	Hit rate: 99.1737
	Miss rate: 0.8263

m = 200
	Hit count: 15963
	Miss count: 133
	Clean evictions: 0
	Dirty evictions: 0
	Total references : 16096
	Hit rate: 99.1737
	Miss rate: 0.8263

==================================================
	lru
--------------------------------------------------
tr-simpleloop
--------------------------------------------------
m = 50
	Hit count: 7793
	Miss count: 2775
	Clean evictions: 93
	Dirty evictions: 2632
	Total references : 10568
	Hit rate: 73.7415
	Miss rate: 26.2585

m = 100
	Hit count: 7891
	Miss count: 2677
	Clean evictions: 0
	Dirty evictions: 2577
	Total references : 10568
	Hit rate: 74.6688
	Miss rate: 25.3312

m = 150
	Hit count: 7893
	Miss count: 2675
	Clean evictions: 0
	Dirty evictions: 2525
	Total references : 10568
	Hit rate: 74.6877
	Miss rate: 25.3123

m = 200
	Hit count: 7893
	Miss count: 2675
	Clean evictions: 0
	Dirty evictions: 2475
	Total references : 10568
	Hit rate: 74.6877
	Miss rate: 25.3123

--------------------------------------------------
tr-blocked
--------------------------------------------------
m = 50
	Hit count: 2519846
	Miss count: 5178
	Clean evictions: 4029
	Dirty evictions: 1099
	Total references : 2525024
	Hit rate: 99.7949
	Miss rate: 0.2051

m = 100
	Hit count: 2521215
	Miss count: 3809
	Clean evictions: 2631
	Dirty evictions: 1078
	Total references : 2525024
	Hit rate: 99.8491
	Miss rate: 0.1509

m = 150
	Hit count: 2521223
	Miss count: 3801
	Clean evictions: 2595
	Dirty evictions: 1056
	Total references : 2525024
	Hit rate: 99.8495
	Miss rate: 0.1505

m = 200
	Hit count: 2521331
	Miss count: 3693
	Clean evictions: 2437
	Dirty evictions: 1056
	Total references : 2525024
	Hit rate: 99.8537
	Miss rate: 0.1463

--------------------------------------------------
tr-matmul
--------------------------------------------------
m = 50
	Hit count: 1926933
	Miss count: 1041219
	Clean evictions: 1040073
	Dirty evictions: 1096
	Total references : 2968152
	Hit rate: 64.9203
	Miss rate: 35.0797

m = 100
	Hit count: 1961708
	Miss count: 1006444
	Clean evictions: 1005269
	Dirty evictions: 1075
	Total references : 2968152
	Hit rate: 66.0919
	Miss rate: 33.9081

m = 150
	Hit count: 2935275
	Miss count: 32877
	Clean evictions: 31654
	Dirty evictions: 1073
	Total references : 2968152
	Hit rate: 98.8923
	Miss rate: 1.1077

m = 200
	Hit count: 2935286
	Miss count: 32866
	Clean evictions: 31593
	Dirty evictions: 1073
	Total references : 2968152
	Hit rate: 98.8927
	Miss rate: 1.1073

--------------------------------------------------
tr-linkedlisttrace
--------------------------------------------------
m = 50
	Hit count: 15865
	Miss count: 231
	Clean evictions: 56
	Dirty evictions: 125
	Total references : 16096
	Hit rate: 98.5649
	Miss rate: 1.4351

m = 100
	Hit count: 15958
	Miss count: 138
	Clean evictions: 0
	Dirty evictions: 38
	Total references : 16096
	Hit rate: 99.1426
	Miss rate: 0.8574

m = 150
	Hit count: 15963
	Miss count: 133
	Clean evictions: 0
	Dirty evictions: 0
	Total references : 16096
	Hit rate: 99.1737
	Miss rate: 0.8263

m = 200
	Hit count: 15963
	Miss count: 133
	Clean evictions: 0
	Dirty evictions: 0
	Total references : 16096
	Hit rate: 99.1737
	Miss rate: 0.8263

==================================================
	clock
--------------------------------------------------
tr-simpleloop
--------------------------------------------------
m = 50
	Hit count: 7780
	Miss count: 2788
	Clean evictions: 106
	Dirty evictions: 2632
	Total references : 10568
	Hit rate: 73.6185
	Miss rate: 26.3815

m = 100
	Hit count: 7890
	Miss count: 2678
	Clean evictions: 1
	Dirty evictions: 2577
	Total references : 10568
	Hit rate: 74.6593
	Miss rate: 25.3407

m = 150
	Hit count: 7893
	Miss count: 2675
	Clean evictions: 0
	Dirty evictions: 2525
	Total references : 10568
	Hit rate: 74.6877
	Miss rate: 25.3123

m = 200
	Hit count: 7892
	Miss count: 2676
	Clean evictions: 0
	Dirty evictions: 2476
	Total references : 10568
	Hit rate: 74.6783
	Miss rate: 25.3217

--------------------------------------------------
tr-blocked
--------------------------------------------------
m = 50
	Hit count: 2519259
	Miss count: 5765
	Clean evictions: 4609
	Dirty evictions: 1106
	Total references : 2525024
	Hit rate: 99.7717
	Miss rate: 0.2283

m = 100
	Hit count: 2520852
	Miss count: 4172
	Clean evictions: 2994
	Dirty evictions: 1078
	Total references : 2525024
	Hit rate: 99.8348
	Miss rate: 0.1652

m = 150
	Hit count: 2521218
	Miss count: 3806
	Clean evictions: 2599
	Dirty evictions: 1057
	Total references : 2525024
	Hit rate: 99.8493
	Miss rate: 0.1507

m = 200
	Hit count: 2521803
	Miss count: 3221
	Clean evictions: 1964
	Dirty evictions: 1057
	Total references : 2525024
	Hit rate: 99.8724
	Miss rate: 0.1276

--------------------------------------------------
tr-matmul
--------------------------------------------------
m = 50
	Hit count: 1926923
	Miss count: 1041229
	Clean evictions: 1040084
	Dirty evictions: 1095
	Total references : 2968152
	Hit rate: 64.9200
	Miss rate: 35.0800

m = 100
	Hit count: 1966370
	Miss count: 1001782
	Clean evictions: 1000607
	Dirty evictions: 1075
	Total references : 2968152
	Hit rate: 66.2490
	Miss rate: 33.7510

m = 150
	Hit count: 2933499
	Miss count: 34653
	Clean evictions: 33428
	Dirty evictions: 1075
	Total references : 2968152
	Hit rate: 98.8325
	Miss rate: 1.1675
	
m = 200
	Hit count: 2935273
	Miss count: 32879
	Clean evictions: 31606
	Dirty evictions: 1073
	Total references : 2968152
	Hit rate: 98.8923
	Miss rate: 1.1077

--------------------------------------------------
tr-linkedlisttrace
--------------------------------------------------
m = 50
	Hit count: 15855
	Miss count: 241
	Clean evictions: 68
	Dirty evictions: 123
	Total references : 16096
	Hit rate: 98.5027
	Miss rate: 1.4973

m = 100
	Hit count: 15955
	Miss count: 141
	Clean evictions: 0
	Dirty evictions: 41
	Total references : 16096
	Hit rate: 99.1240
	Miss rate: 0.8760

m = 150
	Hit count: 15963
	Miss count: 133
	Clean evictions: 0
	Dirty evictions: 0
	Total references : 16096
	Hit rate: 99.1737
	Miss rate: 0.8263

m = 200
	Hit count: 15963
	Miss count: 133
	Clean evictions: 0
	Dirty evictions: 0
	Total references : 16096
	Hit rate: 99.1737
	Miss rate: 0.8263

==================================================
	opt
--------------------------------------------------
tr-simpleloop
--------------------------------------------------
m = 50
	Hit count: 7793
	Miss count: 2775
	Clean evictions: 94
	Dirty evictions: 2631
	Total references : 10568
	Hit rate: 73.7415
	Miss rate: 26.2585

m = 100
	Hit count: 7892
	Miss count: 2676
	Clean evictions: 0
	Dirty evictions: 2576
	Total references : 10568
	Hit rate: 74.6783
	Miss rate: 25.3217

m = 150
	Hit count: 7894
	Miss count: 2674
	Clean evictions: 0
	Dirty evictions: 2524
	Total references : 10568
	Hit rate: 74.6972
	Miss rate: 25.3028

m = 200
	Hit count: 7894
	Miss count: 2674
	Clean evictions: 0
	Dirty evictions: 2474
	Total references : 10568
	Hit rate: 74.6972
	Miss rate: 25.3028

--------------------------------------------------
tr-blocked
--------------------------------------------------
m = 50
	Hit count: 2519828
	Miss count: 5196
	Clean evictions: 4047
	Dirty evictions: 1099
	Total references : 2525024
	Hit rate: 99.7942
	Miss rate: 0.2058


--------------------------------------------------
tr-matmul
--------------------------------------------------

--------------------------------------------------
tr-linkedlisttrace
--------------------------------------------------
m = 50
	Hit count: 15866
	Miss count: 230
	Clean evictions: 56
	Dirty evictions: 124
	Total references : 16096
	Hit rate: 98.5711
	Miss rate: 1.4289

m = 100
	Hit count: 15958
	Miss count: 138
	Clean evictions: 0
	Dirty evictions: 38
	Total references : 16096
	Hit rate: 99.1426
	Miss rate: 0.8574

m = 150
	Hit count: 15963
	Miss count: 133
	Clean evictions: 0
	Dirty evictions: 0
	Total references : 16096
	Hit rate: 99.1737
	Miss rate: 0.8263

m = 200
	Hit count: 15963
	Miss count: 133
	Clean evictions: 0
	Dirty evictions: 0
	Total references : 16096
	Hit rate: 99.1737
	Miss rate: 0.8263




algorithms comparison: 

From the results obtained, we can get some conclusions with respect to memory size and algorithm. For individual algorithm on different memory sizes, most algorithms show the trend that with the increment of the memory sizes, the hit rate is increasing and clean/dirty evictions are decreasing. This is understandable, since larger memory size helps capture more memory hits, thus reducing the times of evictions. rand, due to the fact that it doesn't pick up an appropriate candidate to swap, doesn't show this trend apparently. LRU, although known to have Belady's anomaly, doesn't show this feature distinctively in the simulation. For the 4th trace tr-linkedlisttrace, eviction becomes 0 when memory size is large enough. In terms of comparison of different algorithms in general, for the small files such as tr-simpleloop, most algorithms are finally stablized at 74.*%; while for large files like tr-matmul and tr-block, opt and clock algorithms both give relatively better performance on hit rates than other algorithms. 

LRU algorithm: 

For files of tr-simpleloop, tr-block and tr-linkedlisttrace, the hit rates are quite similar with respect to different memory sizes, while for tr-matmul, the hit rates increase from 63% to 98% when memory size changes from 100 to 150. Even though Belady's anomaly is not obvious in the simulation, we can still find the increased memory size doesn't certainly cause the better hit rates and eviction performance. 
