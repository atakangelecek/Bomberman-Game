#include <iostream>
#include <vector>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include "message.h"
#include "logging.h"
#include "logging.c"
#include "message.c"
#include <errno.h>
#include <string.h>
using namespace std;
int main(){
    int x;
    string s;
    vector<int> map_info;
   
    // map_info[0] = map width -- map_info[1] = map height -- map_info[2] = number of obstacles -- map_info[3] = number of bombers 
    for(int i = 0; i < 4; i++){    
        cin >> x;
        map_info.push_back(x);
    }
    
    vector<vector<int>> obstacles;
    for(int i = 0; i < map_info[2]; i++){
        vector<int> obstacle; // obstacle[0] = x -- obstacle[1] = y -- obstacle[2] = durability
        for(int j = 0; j < 3; j++){
            cin >> x;
            obstacle.push_back(x);
        }
        obstacles.push_back(obstacle);
        obstacle.clear();
    }

    vector<vector<int>> bombers;
    vector<vector<string>> bomber_arguments;
    for(int i = 0; i < map_info[3]; i++){
        vector<int> bomber; // bomber[0] = x -- bomber[1] = y -- bomber[2] = no_of_argument -- bomber[3] = dead or alive
        for(int j = 0; j < 3; j++){
            cin >> x;
            bomber.push_back(x);
        }
        bomber.push_back(1); // alive
        bomber.push_back(1); // for reading the bomber pipe
        bombers.push_back(bomber);
        
        vector<string> arguments;
        for(int j = 0; j < bomber[2]; j++){
            if(j == 0){
                cin >> s;
                arguments.push_back(s);
            }
            else{
                cin >> x;
                string str_num = to_string(x);
                arguments.push_back(str_num);
            }
        }
        bomber_arguments.push_back(arguments); 
        bomber.clear();
        arguments.clear();
    }
    vector<vector<int>> bombs; // bombs[i][0] = time until explosion -- bombs[i][1] = radius of explosion -- bombs[i][2] = x -- bombs[i][3] = y
    vector<int*> bomb_pipes_1;
    vector<int*> bomb_pipes_2;
    vector<int*> pipe_fd_1(map_info[3]);
    vector<int*> pipe_fd_2(map_info[3]);
    //int **pipe_fd_1 = new int*[map_info[3]]; // pipe_fd[0] = ilk pipe -- pipe_fd[1] = ikinci pipe -- pipe_fd[2] = üçüncü pipe ...
    //int **pipe_fd_2 = new int*[map_info[3]]; // pipe_fd[0] = ilk pipe -- pipe_fd[1] = ikinci pipe -- pipe_fd[2] = üçüncü pipe ...
    for(int i = 0; i < map_info[3]; i++){
        pipe_fd_1[i] = new int[2];
    }
    for(int i = 0; i < map_info[3]; i++){
        pipe_fd_2[i] = new int[2];
    }
    vector<pid_t> pid; // pid[0] = bomber1 -- pid[1] = bomber2 -- pid[2] = bomber3 ...
    vector<pid_t> bomb_pid;
    int counter = 0;
    int alive_count = map_info[3];
    int size = map_info[3];
    for(int i = 0; i < map_info[3]; i++){
        if(pipe(pipe_fd_1[i]) == -1){
            cout << "Error creating pipe" << endl;
            exit(1);
        }
        if(pipe(pipe_fd_2[i]) == -1){
            cout << "Error creating pipe" << endl;
            exit(1);
        }
    }
    for (int i = 0; i < pipe_fd_1.size(); i++) {
        pid.push_back(fork());
        if (pid[i] == -1) {
            cerr << "Error creating child process " << i << endl;
            return 1;
        }
        else if (pid[i] == 0) { // Child process

            // redirect stdin and stdout to the pipe
            dup2(pipe_fd_1[i][0], STDIN_FILENO);
            dup2(pipe_fd_2[i][1], STDOUT_FILENO);
            
            // close the unused pipe ends
            close(pipe_fd_1[i][1]);
            close(pipe_fd_2[i][0]);

            // execute the program
            vector<char*> args;
            for(int j=0; j < bomber_arguments[i].size(); j++){
                args.push_back((char *)(bomber_arguments[i][j]).c_str());
            }
            args.push_back(NULL);

            execvp(args[0], args.data());
            // if execvp returns, there was an error
            cerr << "Error executing " << endl;
            return 1;
        }
        else if(pid[i] > 0){
            // close the unused pipe ends
            close(pipe_fd_1[i][0]);
            close(pipe_fd_2[i][1]);
        }
    }
        // Parent process
        // pipe_fd_2[0] read end ----- pipe_fd_1[1] write end
        // alive count 1 iken bütün bomberları dinlemem lazım
        
        bool first = true;
        while((alive_count > 1) || (counter != map_info[3])){

            // Select or poll the bomb pipes to see there is any input 
            fd_set readfds;
            for(int i = 0; i < bomb_pipes_2.size(); i++){
                FD_SET(bomb_pipes_2[i][0], &readfds);
            }
            int maxx_fd = 0;
            for(int i = 0; i < bomb_pipes_2.size(); i++){
                if(bomb_pipes_2[i][0] > maxx_fd){
                    maxx_fd = bomb_pipes_2[i][0];
                }
            }
            if(bomb_pipes_2.size() > 0){
                int readyy = select(maxx_fd + 1, &readfds, NULL, NULL, NULL);
                if(readyy == -1){
                    cout << "Error in select" << endl;
                    exit(1);
                }
                else if(readyy == 0){
                    cout << "Timeout" << endl;
                    exit(1);
                }
                else{
                    for(int i = 0; i < bomb_pipes_2.size(); i++){
                        if(bombs[i][4] == 0){
                            continue;
                        }
                        if(FD_ISSET(bomb_pipes_2[i][0], &readfds)){
                            im incoming_message_bomb;
                            om sent_message_bomb;
                            
                            int re = read(bomb_pipes_2[i][0], &incoming_message_bomb, sizeof(incoming_message_bomb));
                            if(re == -1){
                                cout << "Error reading from pipe" << endl;
                                exit(1);
                            }

                            else if(re <= 0){
                                ;
                            }
                            else if(alive_count == 1){
                                ;
                            }
                            else if(incoming_message_bomb.type == BOMB_EXPLODE){
                                imp incomingMessagePrint;
                                obsd obstacle_info;
                                incomingMessagePrint.pid = bomb_pid[i];
                                incomingMessagePrint.m = &incoming_message_bomb;
                                print_output(&incomingMessagePrint, NULL, NULL, NULL);
                                // bombs[i][0] = time until explosion -- bombs[i][1] = radius of explosion -- bombs[i][2] = x -- bombs[i][3] = y
                                int x_coor = bombs[i][2];
                                int y_coor = bombs[i][3];
                                int radius = bombs[i][1];
                                
                                int up_y = y_coor; // up_y aşağı doğru
                                int down_y = y_coor; // down_y yukarı doğru
                                int left_x = x_coor;
                                int right_x = x_coor;
                                while(radius > 0){
                                    if(right_x < (map_info[0] - 1)){
                                        right_x++;
                                    }
                                    if(left_x > 0){
                                        left_x--;
                                    }
                                    if(up_y < (map_info[1] - 1)){
                                        up_y++;
                                    }
                                    if(down_y > 0){
                                        down_y--;
                                    }
                                    radius--;
                                }

                                for(int i=x_coor; i<=right_x; i++){
                                    for(int j=0; j<obstacles.size(); j++){
                                        if((obstacles[j][0] == i) && (obstacles[j][1] == y_coor)){
                                            right_x = i-1;
                                            obstacle_info.position.x = obstacles[j][0];
                                            obstacle_info.position.y = obstacles[j][1];
                                            obstacle_info.remaining_durability = -1;
                                            if(obstacles[j][2] != -1){ // if it is damageable
                                                obstacles[j][2]--;
                                                obstacle_info.remaining_durability = obstacles[j][2];
                                                if(obstacles[j][2] == 0){
                                                    obstacles.erase(obstacles.begin() + j);
                                                }
                                            }
                                            print_output(NULL,NULL, &obstacle_info, NULL);
                                            break;
                                        }
                                    }
                                }
                                for(int i=x_coor; i>=left_x; i--){
                                    for(int j=0; j<obstacles.size(); j++){
                                        if((obstacles[j][0] == i) && (obstacles[j][1] == y_coor)){
                                            left_x = i+1;
                                            obstacle_info.position.x = obstacles[j][0];
                                            obstacle_info.position.y = obstacles[j][1];
                                            obstacle_info.remaining_durability = -1;
                                            if(obstacles[j][2] != -1){ // if it is damageable
                                                obstacles[j][2]--;
                                                obstacle_info.remaining_durability = obstacles[j][2];
                                                if(obstacles[j][2] == 0){
                                                    obstacles.erase(obstacles.begin() + j);
                                                }
                                            }
                                            print_output(NULL,NULL, &obstacle_info, NULL);
                                            break;
                                        }
                                    }
                                }
                                for(int i=y_coor; i<=up_y; i++){
                                    for(int j=0; j<obstacles.size(); j++){
                                        if((obstacles[j][0] == x_coor) && (obstacles[j][1] == i)){
                                            up_y = i-1;
                                            obstacle_info.position.x = obstacles[j][0];
                                            obstacle_info.position.y = obstacles[j][1];
                                            obstacle_info.remaining_durability = -1;
                                            if(obstacles[j][2] != -1){ // if it is damageable
                                                obstacles[j][2]--;
                                                obstacle_info.remaining_durability = obstacles[j][2];
                                                if(obstacles[j][2] == 0){
                                                    obstacles.erase(obstacles.begin() + j);
                                                }
                                            }
                                            print_output(NULL,NULL, &obstacle_info, NULL);
                                            break;
                                        }
                                    }
                                }
                                for(int i=y_coor; i>=down_y; i--){
                                    for(int j=0; j<obstacles.size(); j++){
                                        if((obstacles[j][0] == x_coor) && (obstacles[j][1] == i)){
                                            down_y = i+1;
                                            obstacle_info.position.x = obstacles[j][0];
                                            obstacle_info.position.y = obstacles[j][1];
                                            obstacle_info.remaining_durability = -1;
                                            if(obstacles[j][2] != -1){ // if it is damageable
                                                obstacles[j][2]--;
                                                obstacle_info.remaining_durability = obstacles[j][2];
                                                if(obstacles[j][2] == 0){
                                                    obstacles.erase(obstacles.begin() + j);
                                                }
                                            }
                                            print_output(NULL,NULL, &obstacle_info, NULL);
                                            break;
                                        }
                                    }
                                }

                                // obstacleları handle ettin şimdi bomberları handle et
                                for(int i=left_x; i<=right_x; i++){
                                    for(int j=0; j< bombers.size(); j++){
                                        if((bombers[j][0] == i) && (bombers[j][1] == y_coor)){
                                            bombers[j][3] = 0; // dead
                                            bombs[j][4] = 0; // dead
                                            if(alive_count > 0)
                                                alive_count--;
                                        }
                                    }            
                                }
                                for(int i=down_y; i<=up_y; i++){
                                    for(int j=0; j< bombers.size(); j++){
                                        if(bombers[j][0] == x_coor && bombers[j][1] == y_coor){
                                            ;
                                        }
                                        else if((bombers[j][0] == x_coor) && (bombers[j][1] == i)){    
                                            bombers[j][3] = 0; // dead
                                            bombs[j][4] = 0; // dead
                                            if(alive_count > 0)
                                                alive_count--;
                                        }
                                    }            
                                }
                                waitpid(bomb_pid[i], NULL, 0);
                            }
                            
                        }
                    }    
                }
            }
            
            //Select or poll the bomber pipes to see there is any input    
                FD_ZERO(&readfds);
                for(int i = 0; i < pipe_fd_2.size(); i++){
                    FD_SET(pipe_fd_2[i][0], &readfds);
                }
                
                int max_fd = 0;
                for(int i = 0; i < pipe_fd_2.size(); i++){
                    if(pipe_fd_2[i][0] > max_fd){
                        max_fd = pipe_fd_2[i][0];
                    }
                }
                
                int ready = select(max_fd + 1, &readfds, NULL, NULL, NULL);
                
                if(ready == -1){
                    cout << "Error in select" << endl;
                    cout << "errno: " << errno << endl;
                    exit(1);
                }
                else if(ready == 0){
                    cout << "Timeout" << endl;
                    exit(1);
                }
                else{
                    
                    for(int i = 0; i < pipe_fd_2.size(); i++){
                        if(bombers[i][4] == 0){
                                continue;
                        }  
                        if(FD_ISSET(pipe_fd_2[i][0], &readfds)){
                            
                            im incoming_message;
                            om sent_message;
                            vector<od> object_data_vector;
                            imp incomingMessagePrint;
                            omp sentMessagePrint;
                                   
                            int r = read(pipe_fd_2[i][0], &incoming_message, sizeof(incoming_message));
                            if(r == -1){
                                cerr << "Error reading from pipe" << endl;
                                exit(1);
                            }
                            if((alive_count == 0) && first){
                                for(int num=0; num<bombers.size(); num++){
                                    if(bombers[num][3] == 0){
                                        bombers[num][3] = 1;
                                    }
                                }
                                first = false;
                            }
                            
                            // if the message is a bomber_start message
                            if(incoming_message.type == BOMBER_START){
                                if(bombers[i][3] == 0){
                                    sent_message.type = BOMBER_DIE;
                                    write(pipe_fd_1[i][1], &sent_message, sizeof(sent_message));
                                    incomingMessagePrint.pid = pid[i];
                                    incomingMessagePrint.m = &incoming_message;
                                    print_output(&incomingMessagePrint, NULL, NULL, NULL);
                                    sentMessagePrint.pid = pid[i];
                                    sentMessagePrint.m = &sent_message;
                                    print_output(NULL, &sentMessagePrint, NULL, NULL);
                                    bombers[i][4] = 0;
                                    waitpid(pid[i], NULL, 0);
                                }
                                else if((alive_count == 1 && bombers[i][3] == 1)){
                                    
                                    sent_message.type = BOMBER_WIN;
                                    write(pipe_fd_1[i][1], &sent_message, sizeof(sent_message));
                                    incomingMessagePrint.pid = pid[i];
                                    incomingMessagePrint.m = &incoming_message;
                                    print_output(&incomingMessagePrint, NULL, NULL, NULL);
                                    sentMessagePrint.pid = pid[i];
                                    sentMessagePrint.m = &sent_message;
                                    print_output(NULL, &sentMessagePrint, NULL, NULL);
                                    counter++;
                                    
                                }
                                else{
                                    sent_message.type = BOMBER_LOCATION;
                                    sent_message.data.new_position.x = bombers[i][0];
                                    sent_message.data.new_position.y = bombers[i][1];
                                    write(pipe_fd_1[i][1], &sent_message, sizeof(sent_message));
                                    incomingMessagePrint.pid = pid[i];
                                    incomingMessagePrint.m = &incoming_message;
                                    print_output(&incomingMessagePrint, NULL, NULL, NULL);
                                    sentMessagePrint.pid = pid[i];
                                    sentMessagePrint.m = &sent_message;
                                    print_output(NULL, &sentMessagePrint, NULL, NULL);
                                }
                                
                            }
                            
                            // if the message is a bomber_move message
                            else if(incoming_message.type == BOMBER_MOVE){
                                
                                if(bombers[i][3] == 0){
                                    
                                    sent_message.type = BOMBER_DIE;
                                    write(pipe_fd_1[i][1], &sent_message, sizeof(sent_message));
                                    incomingMessagePrint.pid = pid[i];
                                    incomingMessagePrint.m = &incoming_message;
                                    print_output(&incomingMessagePrint, NULL, NULL, NULL);
                                    sentMessagePrint.pid = pid[i];
                                    sentMessagePrint.m = &sent_message;
                                    print_output(NULL, &sentMessagePrint, NULL, NULL);
                                    
                                    counter++;
                                    
                                    //bombers.erase(bombers.begin()+i);
                                    bombers[i][4] = 0;
                                    waitpid(pid[i], NULL, 0);
                                }
                                else if(alive_count == 1 && bombers[i][3] == 1){
                                    
                                    sent_message.type = BOMBER_WIN;
                                    write(pipe_fd_1[i][1], &sent_message, sizeof(sent_message));
                                    incomingMessagePrint.pid = pid[i];
                                    incomingMessagePrint.m = &incoming_message;
                                    print_output(&incomingMessagePrint, NULL, NULL, NULL);
                                    sentMessagePrint.pid = pid[i];
                                    sentMessagePrint.m = &sent_message;
                                    print_output(NULL, &sentMessagePrint, NULL, NULL);
                                    if(alive_count == 1){
                                        counter++;
                                    }
                                }
                                else{ 
                                    sent_message.type = BOMBER_LOCATION;
                                    int target_x = incoming_message.data.target_position.x;
                                    int target_y = incoming_message.data.target_position.y;
                                    int flag = 1;
                                    // if position is only one step away from the bomber position
                                    if( (abs(target_x - bombers[i][0]) == 1 && abs(target_y - bombers[i][1]) == 0) || (abs(target_x - bombers[i][0]) == 0 && abs(target_y - bombers[i][1]) == 1) ){
                                        // there should be no obstacles or bombers in the target position
                                        for(vector<int>bomber: bombers){
                                            if((bomber[0] == target_x) && (bomber[1] == target_y)){
                                                flag = 0;
                                                break;
                                            }
                                        }
                                        for(vector<int>obstacle: obstacles){
                                            if((obstacle[0] == target_x) && (obstacle[1] == target_y)){
                                                flag = 0;
                                                break;
                                            }
                                        }
                                        // the target position should not be out of bounds
                                        if((target_x < 0) || (target_x >= map_info[0]) || (target_y < 0) || (target_y >= map_info[1])){
                                            
                                            flag = 0;
                                        }
                                        if(flag == 1){
                                            sent_message.data.new_position.x = target_x;
                                            sent_message.data.new_position.y = target_y;
                                            bombers[i][0] = target_x;
                                            bombers[i][1] = target_y;
                                            incoming_message.data.target_position.x = bombers[i][0];
                                            incoming_message.data.target_position.y = bombers[i][1];
                                            write(pipe_fd_1[i][1], &sent_message, sizeof(sent_message));
                                        }
                                        else if(flag == 0){
                                            sent_message.data.new_position.x = bombers[i][0];
                                            sent_message.data.new_position.y = bombers[i][1];
                                            write(pipe_fd_1[i][1], &sent_message, sizeof(sent_message));
                                        }
                                    }
                                    incomingMessagePrint.pid = pid[i];
                                    incomingMessagePrint.m = &incoming_message;
                                    print_output(&incomingMessagePrint, NULL, NULL, NULL);
                                    sentMessagePrint.pid = pid[i];
                                    sentMessagePrint.m = &sent_message;
                                    print_output(NULL, &sentMessagePrint, NULL, NULL);
                                }


                            }

                            // if message is bomber see
                            else if(incoming_message.type == BOMBER_SEE){
                                
                                if(bombers[i][3] == 0){
                                    
                                    sent_message.type = BOMBER_DIE;
                                    write(pipe_fd_1[i][1], &sent_message, sizeof(sent_message));
                                    incomingMessagePrint.pid = pid[i];
                                    incomingMessagePrint.m = &incoming_message;
                                    print_output(&incomingMessagePrint, NULL, NULL, NULL);
                                    sentMessagePrint.pid = pid[i];
                                    sentMessagePrint.m = &sent_message;
                                    print_output(NULL, &sentMessagePrint, NULL, NULL);
                                    
                                    counter++;
                                    
                                    //bombers.erase(bombers.begin()+i);
                                    bombers[i][4] = 0;
                                    waitpid(pid[i], NULL, 0);
                                }
                                else if(alive_count == 1 && bombers[i][3] == 1){
                                    //cerr << "Bomber is winner" << endl;
                                    sent_message.type = BOMBER_WIN;
                                    write(pipe_fd_1[i][1], &sent_message, sizeof(sent_message));
                                    write(pipe_fd_1[i][1], &sent_message, sizeof(sent_message));
                                    incomingMessagePrint.pid = pid[i];
                                    incomingMessagePrint.m = &incoming_message;
                                    print_output(&incomingMessagePrint, NULL, NULL, NULL);
                                    sentMessagePrint.pid = pid[i];
                                    sentMessagePrint.m = &sent_message;
                                    print_output(NULL, &sentMessagePrint, NULL, NULL);
                                    if(alive_count == 1){
                                        counter++;
                                    }
                                }
                                else{
                                    od object_data;
                                    int x_coord = bombers[i][0];
                                    int y_coord = bombers[i][1];
                                    // The bombers as mentioned before can see three steps into all four directions unless it is blocked by an obstacle.
                                    int up_y = y_coord; // up_y aşağı doğru
                                    int down_y = y_coord; // down_y yukarı doğru
                                    int left_x = x_coord;
                                    int right_x = x_coord;
                                    int range = 3;
                                    while(range > 0){
                                        if(right_x < (map_info[0] - 1)){
                                            right_x++;
                                        }
                                        if(left_x > 0){
                                            left_x--;
                                        }
                                        if(up_y < (map_info[1] - 1)){
                                            up_y++;
                                        }
                                        if(down_y > 0){
                                            down_y--;
                                        }
                                        range--;
                                    }
                                    for(int i=x_coord; i<=right_x; i++){
                                        for(vector<int>obstacle: obstacles){
                                            if((obstacle[0] == i) && (obstacle[1] == y_coord)){
                                                right_x = i-1;
                                                break;
                                            }
                                        }
                                    }
                                    for(int i=x_coord; i>=left_x; i--){
                                        for(vector<int>obstacle: obstacles){
                                            if((obstacle[0] == i) && (obstacle[1] == y_coord)){
                                                left_x = i+1;
                                                break;
                                            }
                                        }
                                    }
                                    for(int i=y_coord; i<=up_y; i++){
                                        for(vector<int>obstacle: obstacles){
                                            if((obstacle[0] == x_coord) && (obstacle[1] == i)){
                                                up_y = i-1;
                                                break;
                                            }
                                        }
                                    }
                                    for(int i=y_coord; i>=down_y; i--){
                                        for(vector<int>obstacle: obstacles){
                                            if((obstacle[0] == x_coord) && (obstacle[1] == i)){
                                                down_y = i+1;
                                                break;
                                            }
                                        }
                                    }
                                    //vector<od> object_data_vector;
                                    for(int i=left_x; i<=right_x; i++){
                                        for(vector<int>bomber: bombers){
                                            if((bomber[0] == x_coord) && (bomber[1] == y_coord)){
                                                ;
                                            }
                                            else if((bomber[0] == i) && (bomber[1] == y_coord) && (bomber[3] == 1)){
                                                object_data.type = BOMBER;
                                                object_data.position.x = i;
                                                object_data.position.y = y_coord;
                                                object_data_vector.push_back(object_data);
                                            }
                                        }
                                        // TODO do the same for bomb vector (DAHA YAPMADIN)
                                        // Some code here
                                        for(int j=0; j<bombs.size(); j++){
                                            if((bombs[j][2] == i) && (bombs[j][3] == y_coord)){
                                                object_data.type = BOMB;
                                                object_data.position.x = i;
                                                object_data.position.y = y_coord;
                                                object_data_vector.push_back(object_data);
                                            }
                                        }
                                    }
                                    for(int j=down_y; j<=up_y; j++){
                                        for(vector<int>bomber: bombers){
                                            if((bomber[0] == x_coord) && (bomber[1] == y_coord)){
                                                ;
                                            }
                                            else if((bomber[0] == x_coord) && (bomber[1] == j) && (bomber[3] == 1)){
                                                object_data.type = BOMBER;
                                                object_data.position.x = x_coord;
                                                object_data.position.y = j;
                                                object_data_vector.push_back(object_data);
                                            }
                                        }
                                        // TODO do the same for bomb vector (DAHA YAPMADIN)
                                        // Some code here
                                        for(int k=0; k<bombs.size(); k++){
                                            if((bombs[k][2] == x_coord) && (bombs[k][3] == y_coord)){
                                                ;
                                            }
                                            else if((bombs[k][2] == x_coord) && (bombs[k][3] == j)){
                                                object_data.type = BOMB;
                                                object_data.position.x = x_coord;
                                                object_data.position.y = j;
                                                object_data_vector.push_back(object_data);
                                            }
                                        }  
                                    }
                                    sent_message.type = BOMBER_VISION;
                                    sent_message.data.object_count = object_data_vector.size();
                                    send_message(pipe_fd_1[i][1], &sent_message);
                                    send_object_data(pipe_fd_1[i][1], object_data_vector.size(), object_data_vector.data());

                                    incomingMessagePrint.pid = pid[i];
                                    incomingMessagePrint.m = &incoming_message;
                                    print_output(&incomingMessagePrint, NULL, NULL, NULL);
                                    sentMessagePrint.pid = pid[i];
                                    sentMessagePrint.m = &sent_message;
                                    print_output(NULL, &sentMessagePrint, NULL, object_data_vector.data());
                                    object_data_vector.clear();
                                }
                        
                            }
                            
                            //if message is bomber plant
                            else if(incoming_message.type == BOMBER_PLANT){
                                
                                if(bombers[i][3] == 0){
                                    
                                    sent_message.type = BOMBER_DIE;
                                    write(pipe_fd_1[i][1], &sent_message, sizeof(sent_message));
                                    incomingMessagePrint.pid = pid[i];
                                    incomingMessagePrint.m = &incoming_message;
                                    print_output(&incomingMessagePrint, NULL, NULL, NULL);
                                    sentMessagePrint.pid = pid[i];
                                    sentMessagePrint.m = &sent_message;
                                    print_output(NULL, &sentMessagePrint, NULL, NULL);
                                    
                                    counter++;
                                    
                                    //bombers.erase(bombers.begin()+i);
                                    bombers[i][4] = 0;
                                    waitpid(pid[i], NULL, 0);
                                }
                                else if(alive_count == 1 && bombers[i][3] == 1){
                                    
                                    sent_message.type = BOMBER_WIN;
                                    write(pipe_fd_1[i][1], &sent_message, sizeof(sent_message));
                                    incomingMessagePrint.pid = pid[i];
                                    incomingMessagePrint.m = &incoming_message;
                                    print_output(&incomingMessagePrint, NULL, NULL, NULL);
                                    sentMessagePrint.pid = pid[i];
                                    sentMessagePrint.m = &sent_message;
                                    print_output(NULL, &sentMessagePrint, NULL, NULL);
                                    if(alive_count == 1){
                                        counter++;
                                    }
                                }
                                else{
                                    int x_coord = bombers[i][0];    
                                    int y_coord = bombers[i][1];
                                    
                                    bool is_failed = false;
                                    for(vector<int>bomb: bombs){
                                        if((bomb[2] == x_coord) && (bomb[3] == y_coord)){
                                            is_failed = true;
                                            break;
                                        }
                                    }
                                    // if bomb is planted successfully
                                    if(is_failed == false){
                                        vector<int> new_bomb;
                                        new_bomb.push_back( incoming_message.data.bomb_info.interval );
                                        new_bomb.push_back(incoming_message.data.bomb_info.radius);
                                        new_bomb.push_back(x_coord);
                                        new_bomb.push_back(y_coord);
                                        new_bomb.push_back(1); // is_alive
                                        bombs.push_back(new_bomb);
                                        new_bomb.clear();

                                        
                                        // reallocate new space to the pipes
                                        bomb_pipes_1.push_back(new int[2]);
                                        bomb_pipes_2.push_back(new int[2]);
                                        
                                        if(pipe(bomb_pipes_1[bomb_pipes_1.size()-1]) == -1){
                                            cerr << "Error creating pipe" << endl;
                                            exit(1);
                                        }
                                        if(pipe(bomb_pipes_2[bomb_pipes_2.size()-1]) == -1){
                                            cerr << "Error creating pipe" << endl;
                                            exit(1);
                                        }
                                        bomb_pid.push_back(fork());
                                        if(bomb_pid[bomb_pid.size()-1] == 0){
                                            // child process
                                            dup2(bomb_pipes_1[bomb_pipes_1.size()-1][0], STDIN_FILENO);
                                            dup2(bomb_pipes_2[bomb_pipes_2.size()-1][1], STDOUT_FILENO);
                                            // close the read end of the pipe
                                            close(bomb_pipes_1[bomb_pipes_1.size()-1][1]);
                                            close(bomb_pipes_2[bomb_pipes_2.size()-1][0]);
                                            // execute the program
                                            vector<char*> args;
                                            string program_name = "./bomb";
                                            args.push_back((char *)(program_name).c_str());
                                            args.push_back((char *)(to_string(incoming_message.data.bomb_info.interval)).c_str());
                                            args.push_back(NULL);
                                            
                                            execvp(args[0], args.data());
                                            // if execvp returns, there was an error
                                            cerr << "Error executing " << args[0] << endl;
                                            return 1;
                                        }
                                        else if(bomb_pid[bomb_pid.size()-1] > 0){
                                            // parent process
                                            // close the write end of the pipe
                                            close(bomb_pipes_1[bomb_pipes_1.size()-1][0]);
                                            close(bomb_pipes_2[bomb_pipes_2.size()-1][1]);
                                            
                                            sent_message.type = BOMBER_PLANT_RESULT;
                                            sent_message.data.planted = 1;

                                            write(pipe_fd_1[i][1], &sent_message, sizeof(sent_message));
                                        }
                                        else{
                                            cout << "Error forking" << endl;
                                            exit(1);
                                        }
                                    }
                                    // if bomb is not planted successfully
                                    else{
                                        sent_message.type = BOMBER_PLANT_RESULT;
                                        sent_message.data.planted = 0;
                                        write(pipe_fd_1[i][1], &sent_message, sizeof(sent_message));
                                    }

                                    incomingMessagePrint.pid = pid[i];
                                    incomingMessagePrint.m = &incoming_message;
                                    print_output(&incomingMessagePrint, NULL, NULL, NULL);
                                    sentMessagePrint.pid = pid[i];
                                    sentMessagePrint.m = &sent_message;
                                    print_output(NULL, &sentMessagePrint, NULL, NULL);
                                }

                            }
                            
                        }
                    }
                }
            usleep(1000);
        }
        
        
        for(int i=0; i<bomb_pid.size(); i++){    
            if(bombs[i][4] == 1){
                im left_bomb_message;
                left_bomb_message.type = BOMB_EXPLODE;
                imp ingoing_message;
                ingoing_message.m = &left_bomb_message;
                ingoing_message.pid = bomb_pid[i];
                print_output(&ingoing_message, NULL, NULL, NULL );
                waitpid(bomb_pid[i], NULL, 0);
            }
        }
        
    
}