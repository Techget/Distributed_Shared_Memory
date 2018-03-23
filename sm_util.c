




long get_number(char *str){
	char *p = str;
	long val;
	while (*p) { // While there are more characters to process...
	    if (isdigit(*p)) { // Upon finding a digit, ...
	        val = strtol(p, &p, 10); // Read a number, ...
	    } else { // Otherwise, move on to the next character.
	        p++;
	    }
	}

	return val;
}