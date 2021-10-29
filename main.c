#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <assert.h>

#define password "enc_pass"
#define empty "empty"
#define send_complete "complete"
#define msg_acknowledge "confirm"

int buff_size = 256;
int MAX = 100000;
int s = 32;
int mod_val = 26;
int convert_val = 65;

/*
 * Function: error
 * -------------------------
 * Prints the exit_message to stderr and exits if exit_command equals one.
 *
 * Parameters:
 * -------------------------
 * *exit_message: the message to be printed to stderr.
 * num: the exit status value.
 * exit_command: if 1, this function will exit, otherwise it will not.
 *
 * Returns:
 * -------------------------
 * void
 */
void error(const char *exit_message, int num, int exit_command) {
    fprintf(stderr, exit_message);
    if (exit_command == 1) {
        exit(num);
    }
}

/*
 * Function: check_credentials
 * -------------------------
 * Compares the two supplied strings. If they don't match return 1, otherwise return 0.
 *
 * Parameters:
 * -------------------------
 * *one: one of the strings to be checked
 * *two: the other string to be checked
 *
 * Returns:
 * -------------------------
 * ret_num: 1 if the strings don't match, 0 otherwise.
 */
int check_credentials(char *one, char *two) {
    int ret_num = 0;

    if(strcmp(one, two)) {
        ret_num = 1;
    }

    return ret_num;
}

/*
 * Function: init_server_socket
 * -------------------------
 * Sets up the socket for the server. Most of this code comes from server.c file provided in the assignment prompt.
 *
 * Parameters:
 * -------------------------
 * Port: the port for the socket to be associated with.
 *
 * Returns:
 * -------------------------
 * A new socket file descriptor.
 */
int init_server_socket(port) {

    struct sockaddr_in address;

    memset((char*) &address, '\0', sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port); // set bit order

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        error("Couldn't open socket!", 2, 1);
    }

    // Associate the socket to the port
    if (bind(sock, (struct sockaddr *) &address,sizeof(address)) < 0){
        error("Couldn't bind!",2, 1);
    }

    listen(sock, 5);
    return sock;
}

/*
 * Function: read_from_socket
 * -------------------------
 * Calls the read() system call to read from a socket to a buffer.
 *
 * Parameters:
 * -------------------------
 *  *buff: a buffer to hold the content.
 *  _sock: the socket number to call read() on.
 *
 * Returns:
 * -------------------------
 * void
 */
void read_from_socket(char *buff, int _sock) {
    int val; // used to confirm read() worked.

    //ensures buff is ready for the text.
    memset(buff, '\0', buff_size);

    // read text into buff.
    val = read(_sock, buff, (buff_size - 1));

    if (val < 0){
        error("Couldn't read from socket!",0, 0);
    }
}

/*
 * Function: send_to_socket
 * -------------------------
 * Sends the content of buff to the socket provided as the parameter "_sock". If msg_bool is 1, buff is erased and the contents of msg are added to it.
 *
 * Parameters:
 * -------------------------
 * _sock: the destination socket.
 * buff: the source buffer whose content is sent to _sock.
 * msg_bool: if this is 1, the contents of msg are sent. If this is 0, they are not.
 * msg: a message sent to _sock.
 *
 * Returns:
 * -------------------------
 * void
 */
void send_to_socket(char *buff, int _sock, int msg_bool, char *msg) {
    int val; // used to confirm write() worked.

    if (msg_bool == 0) {
        // send text to _sock
        val = write(_sock, buff, strlen(buff));
    } else if (msg_bool == 1) {
        // clear buffer and add msg to it
        memset(buff, '\0', buff_size);
        strcpy(buff, msg);
        // send text to _sock
        val = write(_sock, buff, strlen(buff));
    }

    // confirmed write() worked.
    if (val < 0){
        error("Couldn't write to socket!", 1, 1);
    }
}

/*
 * Function: get_from_client
 * -------------------------
 * gets content from the client socket and adds it to the content buffer.
 *
 * Parameters:
 * -------------------------
 * content: the buffer to receive the text from the client.
 * sock: the client socket number.
 *
 * Returns:
 * -------------------------
 * void
 */
void get_from_client(char *content, int sock) {

    char temp_buff[buff_size];
    int loop = 1;

    while(loop) {

        // gets content from client
        read_from_socket(temp_buff, sock);

        // add content to the buffer
        if(strcmp(temp_buff, send_complete) != 0) {
            strcat(content, temp_buff);
            // if the end signal is found, end loop.
        } else {
            loop = 0;
        }

        send_to_socket(temp_buff, sock, 1, msg_acknowledge);
    }
}

/*
 * Function: int_to_letter
 * -------------------------
 * Converts an encrypted integer value to its ASCII integer value.
 *
 * Parameters:
 * -------------------------
 * convert: the value to be converted
 *
 * Returns:
 * -------------------------
 * return_value: the converted value
 */
int int_to_letter(int convert) {
    int return_value;
    if (convert == 33) {
        return_value = s;
    } else {
        return_value = convert + convert_val;
        if (return_value < 0) {
            return_value = s;
        }
    }
    return return_value;
}

/*
 * Function: letter_to_int
 * -------------------------
 * Converts the letter ASCII integer value to another integer value.
 *
 * Parameters:
 * -------------------------
 * convert: the character to be converted
 *
 * Returns:
 * -------------------------
 * return_value: the resulting integer value
 */
int letter_to_int(char convert) {
    int return_value;
    if (convert == s) {
        return_value = s;
    } else {
        return_value = convert - convert_val; // convert to A = 0 system.
    }
    return return_value;
}

/*
 * Function: encrypt_it
 * -------------------------
 * Takes an unencrpyted character and encrypts using the key character passed in.
 *
 * Parameters:
 * -------------------------
 * unencrypted: the unencrypted char to be encrypted.
 * key: the key used to encrypt.
 *
 * Returns:
 * -------------------------
 * encrypted: the resulting encrypted character
 */
char encrypt_it(char unencrypted, char key) {
    char encrypted; // return char

    // changes the unencrypted char to A = 0 indexed system.
    int letter_val = letter_to_int(unencrypted);
    // changes the key char to A = 0 indexed system.
    int key_val = letter_to_int(key);
    // encrypts
    int result = (letter_val + key_val);
    result = result % mod_val;
    if (letter_val == s) {
        encrypted = s;
    } else {
        encrypted = int_to_letter(result);
    }
    return encrypted;
}

/*
 * Function: encode
 * -------------------------
 * Using the key, encrypts the raw text char by char, and stores the encrypted text in result.
 *
 * Parameters:
 * -------------------------
 * raw_text: the text to be encrypted
 * key: the key used to encrypt the text
 * result: the resulting encrypted text
 *
 * Returns:
 * -------------------------
 * void
 */
void encode(char *raw_text, char *key, char *result) {
    for (int counter = 0; counter < strlen(raw_text); counter++) {
        if (raw_text[counter] != '\0') {
            if (raw_text[counter] != '\n') {
                result[counter] = encrypt_it(raw_text[counter], key[counter]);
            }
        } else {
            result[counter] = '\0';
        }
    }
}

/*
 * Function: send_text
 * -------------------------
 * Sends the encrypted text to the socket.
 *
 * Parameters:
 * -------------------------
 * encrypted_text: the buffer containing the encrypted text
 * socket_dest: the destination for the encrypted text
 *
 * Returns:
 * -------------------------
 * void
 */
void send_text(char *encrypted_text, int socket_dest) {
    FILE *file_pointer;
    int enc_txt_len = strlen(encrypted_text) + 1;

    // using a file pointer to make it easier to use fgets.
    file_pointer = fmemopen(encrypted_text, enc_txt_len,"r");
    assert(file_pointer != NULL);

    char tmp_buff[buff_size];
    memset(tmp_buff, '\0', buff_size);

    while(fgets(tmp_buff, (buff_size - 1), file_pointer)) {
        send_to_socket(tmp_buff, socket_dest, 0, empty);
        memset(tmp_buff, '\0', buff_size);
        read_from_socket(tmp_buff, socket_dest);
        memset(tmp_buff, '\0', buff_size);
    }

    // send confirmation that there's nothing more to send
    send_to_socket(tmp_buff, socket_dest, 1, send_complete);
    // get confirmation that this message was received
    memset(tmp_buff, '\0', buff_size);
    read_from_socket(tmp_buff, socket_dest);
    fclose(file_pointer);
}

/*
 * Function: get_files_from_client
 * -------------------------
 * Gets the required files from the client at the correct socket and stores the results in the respective buffers.
 *
 * Parameters:
 * -------------------------
 * sock: the socket to be used
 * _text: the buffer to hold the plain text
 * _key: the buffer for the key
 *
 * Returns:
 * -------------------------
 * void
 */
void get_files_from_client(int _sock, char *_text, char *_key) {
    get_from_client(_text, _sock);
    get_from_client(_key, _sock);
}

/*
 * Function: get_encrypt_send
 * -------------------------
 * Gets the text and key files from the client, encrypts the text, and sends the encrypted text back to the client.
 *
 * Parameters:
 * -------------------------
 * sock: the socket to be used for communication with the client
 *
 * Returns:
 * -------------------------
 * void
 */
void get_encrypt_send(int sock) {
    char text[MAX], key[MAX], cipher[MAX];
    get_files_from_client(sock, text, key);
    encode(text, key, cipher);
    send_text(cipher, sock);
}

/*
 * Function: verify_correct_connection
 * -------------------------
 * Verifies that the correct client is connected.
 *
 * Parameters:
 * -------------------------
 * sock: the socket to be used for the connection.
 *
 * Returns:
 * -------------------------
 * void
 */
void verify_correct_connection(int sock) {
    char temp_buff[buff_size];
    memset(temp_buff, '\0', buff_size);

    // RECEIVE THE PASSWORD FROM THE CLIENT
    read_from_socket(temp_buff, sock);

    // CONFIRM CLIENT SUPPLIED PASSWORD IS CORRECT
    if(check_credentials(temp_buff, password) == 1) {
        close(sock);
        error("Cannot connect!\n",0, 0);
    }

    // SEND PASSWORD TO CLIENT SO CLIENT CAN VERIFY CORRECT SERVER
    send_to_socket(temp_buff, sock, 1, password);
}

int main(int argc, char *argv[]) {
    int client_socket, true;
    true = 1;
    socklen_t sizeof_client_address;
    struct sockaddr_in address_of_client;

    // Check usage & args
    if (argc < 2) {
        fprintf(stderr,"USAGE: ./enc_server text_file key_file port_number\n");
        exit(1);
    }

    // Sets up the server socket
    int server_socket = init_server_socket(atoi(argv[1]));

    while(true) {

        sizeof_client_address = sizeof(address_of_client);

        client_socket = accept(server_socket,(struct sockaddr *) &address_of_client, &sizeof_client_address);

        if (client_socket < 0){
            error("accept() failed! Client socket not ready!\n",0, 0);
        }

        pid_t spawnPid = -5;

        // FORK!
        spawnPid = fork();

        // failed fork branch
        if (spawnPid == -1) {
            printf("--Fork failed!!!---\n");
            fflush(stdout);
            printf("Spawn PID = %d", spawnPid);
            fflush(stdout);
            exit(1);
        }

        // child branch
        else if (spawnPid == 0) {
            verify_correct_connection(client_socket);
            get_encrypt_send(client_socket);
            close(client_socket);
            exit(0);
        }

        // parent
        else {
            close(client_socket);
        }
    }
    close(server_socket);
    return EXIT_SUCCESS;
}
