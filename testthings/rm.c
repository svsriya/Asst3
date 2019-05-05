#include<stdio.h> 
  
int main( int argc, char** argv ) 
{ 
   if (remove(argv[1]) == 0) 
      printf("Deleted successfully"); 
   else
      printf("Unable to delete the file"); 
  
   return 0; 
}
