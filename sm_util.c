



/* Extract oct number from string 
*/
long get_number(char *str){
	char *p = str;
	long val;
	while (*p) { 
	    if (isdigit(*p)) { 
	        val = strtol(p, &p, 10); 
	    } else { 
	        p++;
	    }
	}

	return val;
}