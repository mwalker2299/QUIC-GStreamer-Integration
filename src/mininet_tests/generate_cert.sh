#!/bin/bash

#!/bin/bash
OUTPUT=""                                   
usage() {                                 
  echo "This command will generate the SSL Certifcate and keys used by the QUIC elements"
  echo "please provide an output dir for the cert and key using -o"
}
exit_abnormal() {                         
  usage
  exit 1
}
while getopts "o:" options; do         
                                          
                                          
  case "${options}" in                    
    o)                                    
      OUTPUT=${OPTARG}                      
      ;;
    :)                                    
      echo "Error: -${OPTARG} requires an argument."
      exit_abnormal                       
      ;;
    *)                                    
      exit_abnormal                       
      ;;
  esac
done
if [ "$OUTPUT" = "" ]; then                 
  usage
  exit 0                                  
fi 

OUTPUT=${OUTPUT%/}

echo "Creating SSL certfile and key used in evaluation"
echo "$OUTPUT"
openssl genrsa 2048 > "${OUTPUT}/host.key"
chmod 400 "${OUTPUT}/host.key"
yes "UK" | openssl req -new -x509 -nodes -sha256 -days 365 -key "${OUTPUT}/host.key" -out "${OUTPUT}/host.cert"