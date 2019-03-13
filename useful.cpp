 

  SRC = opendir(argv[2]);
  if (SRC == NULL) {
    fprintf(stderr,"Error opening source directory %s\n", argv[2]);     
    exit(8);
  }
