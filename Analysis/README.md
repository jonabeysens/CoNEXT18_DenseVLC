To reproduce the results in from the CoNEXT '18 publication, follow these steps:
- open Matlab and add the folder Analysis and all its subfolders to the path
- open the folder simulations_results or experimental_results, depending on which result you want to reproduce
- open the script in Matlab named with the figure you want to create
- choose whether you want to load a data set (to get exactly the same figure as in the paper), or acquire a new data set by the variable "acquire"
- run the script and wait for the figures to appear

Notes: 
- All code is written in Matlab R2017b. If you use another version, it may be that some functions are not supported
- In all the scripts, the placement of the TXs is loaded. If you want to create this placement yourself, you can run the script "main_tx_placement.m" in the helper_functions folder
- The folder data contains the data sets used for the submission. The folder components includes classes, needed to run the scrips. The folder helper_functions contains extra functions used in the script.
