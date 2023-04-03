#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <poll.h>

#include "message.h"
#include "logging.h"

#define PIPE(fd) socketpair(AF_UNIX, SOCK_STREAM, PF_UNIX, fd)

typedef struct bomber
{
    coordinate position;
    int n_args;
    char **argv;
} bomber;

int sum(char *arr, int len)
{
    int sum;

    while (len--)
    {
        sum += *arr++;
    }

    return sum;
}

int main(int argc, char *argv[])
{
    // Parse the input
    int map_width, map_height, obstacle_count, bomber_count, *bombers_alive, bombers_alive_count = 0, bomb_count, *bombs_active, bombs_active_count = 0, winner = -1;
    obsd *obstacles;
    bomber *bombers;
    pid_t *bomber_pids;
    bd *bombs;
    pid_t *bomb_pids;
    coordinate *bomb_position;

    scanf("%d %d %d %d", &map_width, &map_height, &obstacle_count, &bomber_count);

    // CREATE MAPS

    char **bomber_map = malloc(map_height * sizeof(char *));
    for (int i = 0; i < map_height; i++)
    {
        bomber_map[i] = calloc(map_width, sizeof(char));
    }

    char **bomb_map = malloc(map_height * sizeof(char *));
    for (int i = 0; i < map_height; i++)
    {
        bomb_map[i] = calloc(map_width, sizeof(char));
    }

    int **obstacle_map = malloc(map_height * sizeof(int *));
    for (int i = 0; i < map_height; i++)
    {
        obstacle_map[i] = calloc(map_width, sizeof(int));
    }

    // OBSTACLES

    obstacles = malloc(obstacle_count * sizeof(obsd));

    for (int i = 0; i < obstacle_count; i++)
    {
        scanf("%d %d %d", &obstacles[i].position.x, &obstacles[i].position.y, &obstacles[i].remaining_durability);
        
        obstacle_map[obstacles[i].position.y][obstacles[i].position.x] = obstacles[i].remaining_durability;
    }

    // BOMBERS

    bombers = malloc(bomber_count * sizeof(bomber));
    bombers_alive = calloc(bomber_count, sizeof(int));
    bomber_pids = calloc(bomber_count, sizeof(int));

    for (int i = 0; i < bomber_count; i++)
    {
        scanf("%d %d %d", &bombers[i].position.x, &bombers[i].position.y, &bombers[i].n_args);

        bombers[i].argv = (char **) malloc((bombers[i].n_args + 1) * sizeof(char *));

        for (int j = 0; j < bombers[i].n_args; j++)
        {
            scanf("%ms", &bombers[i].argv[j]);
        }

        bombers[i].argv[bombers[i].n_args] = NULL;

        bomber_map[bombers[i].position.y][bombers[i].position.x] = 1;
    }

    // free(obstacles);

    // for (int i = 0; i < bomber_count; i++)
    // {
    //     for (int j = 0; j < bombers[i].n_args + 1; j++)
    //     {
    //         free(bombers[i].argv[j]);
    //     }

    //     free(bombers[i].argv);
    // }

    // free(bombers);

    // Create pipes for bomber processes and fork them
    int (*bomber_fds)[2];
    int (*bomb_fds)[2];

    bomber_fds = malloc(bomber_count * sizeof(int) * 2);

    for (int i = 0; i < bomber_count; i++)
    {
        PIPE(bomber_fds[i]);

        if (bomber_pids[i] = fork())
        {
            bombers_alive[i] = 1;
            bombers_alive_count++;
        }
        else
        {
            dup2(bomber_fds[i][0], 0);
            close(bomber_fds[i][0]);
            dup2(bomber_fds[i][1], 1);
            close(bomber_fds[i][1]);

            execv("./bomber", bombers[i].argv);
        }
    }

    while (bombers_alive_count)
    {
        for (int i = 0; i < bomb_count; i++)
        {
            if (!bombs_active[i])
            {
                continue;
            }
            else
            {
                struct pollfd in = {bomb_fds[i][0]};
                if (poll(&in, 1, 0))
                {
                    im in_message;
                    read_data(bomb_fds[i][0], &in_message);

                    imp in_message_print = {bomb_pids[i], &in_message};

                    print_output(&in_message_print, NULL, NULL, NULL);

                    om out_message;

                    if (in_message.type == BOMB_EXPLODE)
                    {
                        // Deactivate bomb
                        bombs_active[i] = 0;
                        bombs_active_count--;

                        int child_status;
                        waitpid(bomb_pids[i], &child_status, 0);

                        close(bomb_fds[i][0]);
                        close(bomb_fds[i][1]);

                        // OBJECTS WITHIN RADIUS

                        // TOP:  in order of decreasing i (according to array indexing)
                        for (unsigned int x = bomb_position[i].x, start_y = bomb_position[i].y,  end_y = start_y - bombs[i].radius; start_y >= 0 && start_y >= end_y; start_y--)
                        {
                            // Check for obtacles
                            if (obstacle_map[start_y][x])
                            {
                                if (obstacle_map[start_y][x] > 0)
                                {
                                    obstacle_map[start_y][x]--;
                                }

                                obsd obstacle_data = {{x, start_y}, obstacle_map[start_y][x]};

                                print_output(NULL, NULL, &obstacle_data, NULL);

                                break;
                            }

                            // Check for fellow bombers
                            if (bomber_map[start_y][x])
                            {
                                bomber_map[start_y][x] = 0;
                            }
                        }

                        // DOWN: in order of increasing i (according to array indexing)
                        for (unsigned int x = bomb_position[i].x, start_y = bomb_position[i].y,  end_y = start_y + bombs[i].radius; start_y < map_height && start_y <= end_y; start_y++)
                        {
                            // Check for obtacles
                            if (obstacle_map[start_y][x])
                            {
                                if (obstacle_map[start_y][x] > 0)
                                {
                                    obstacle_map[start_y][x]--;
                                }

                                obsd obstacle_data = {{x, start_y}, obstacle_map[start_y][x]};

                                print_output(NULL, NULL, &obstacle_data, NULL);

                                break;
                            }

                            // Check for fellow bombers
                            if (bomber_map[start_y][x])
                            {
                                bomber_map[start_y][x] = 0;
                            }
                        }

                        // RIGHT: in order of increasing j (according to array indexing)
                        for (unsigned int start_x = bomb_position[i].x, y = bomb_position[i].y,  end_x = start_x - bombs[i].radius; start_x <= map_width && start_x <= end_x; start_x++)
                        {
                            // Check for obtacles
                            if (obstacle_map[y][start_x])
                            {
                                if (obstacle_map[y][start_x] > 0)
                                {
                                    obstacle_map[y][start_x]--;
                                }

                                obsd obstacle_data = {{start_x, y}, obstacle_map[y][start_x]};

                                print_output(NULL, NULL, &obstacle_data, NULL);

                                break;
                            }

                            // Check for fellow bombers
                            if (bomber_map[y][start_x])
                            {
                                bomber_map[y][start_x] = 0;
                            }
                        }

                        // LEFT: in order of decreasing j (according to array indexing)
                        for (unsigned int start_x = bomb_position[i].x, y = bomb_position[i].y,  end_x = start_x - bombs[i].radius; start_x >= 0 && start_x >= end_x; start_x--)
                        {
                            // Check for obtacles
                            if (obstacle_map[y][start_x])
                            {
                                if (obstacle_map[y][start_x] > 0)
                                {
                                    obstacle_map[y][start_x]--;
                                }

                                obsd obstacle_data = {{start_x, y}, obstacle_map[y][start_x]};

                                print_output(NULL, NULL, &obstacle_data, NULL);

                                break;
                            }

                            // Check for fellow bombers
                            if (bomber_map[y][start_x])
                            {
                                bomber_map[y][start_x] = 0;
                            }
                        }

                        // Check for the case where only one bomber remains after explosion
                        unsigned int remaining_bombers;
                        for (int i = 0; i < map_height; i++)
                        {
                            remaining_bombers += sum(bomber_map[i], map_width);
                        }

                        if (remaining_bombers == 1)
                        {
                            for (int i = 0; i < bomber_count; i++)
                            {
                                if (bomber_map[bombers[i].position.y][bombers[i].position.x])
                                {
                                    winner = i;
                                    break;
                                }
                            }
                        }

                        // Check for the case where no bomber remains after explosion
                        if (remaining_bombers == 0)
                        {
                            int bomber_furthest_away;
                            unsigned int distance = -1;

                            for (int j = 0; i < bomber_count; i++)
                            {
                                if (bombers_alive[j])
                                {
                                    if (abs(bomb_position[i].x - bombers[j].position.x) + abs(bomb_position[i].y - bombers[i].position.y) > distance)
                                    {
                                        distance = abs(bomb_position[i].x - bombers[j].position.x) + abs(bomb_position[i].y - bombers[j].position.y);

                                        bomber_furthest_away = j;
                                    }
                                }
                            }

                            winner = bomber_furthest_away;
                        }
                    }
                }
            }
        }

        for (int i = 0; i < bomber_count; i++)
        {
            struct pollfd in = {bomber_fds[i][0]};
            if (bombers_alive[i] && poll(&in, 1, 0))
            {
                im in_message;
                read_data(bomber_fds[i][0], &in_message);

                imp in_message_print = {bomber_pids[i], &in_message};

                print_output(&in_message_print, NULL, NULL, NULL);

                om out_message;

                // If explosion killed bomber, send it a death message
                if (!bomber_map[bombers[i].position.y][bombers[i].position.x] || i == winner)
                {
                    bombers_alive[i] = 0;
                    bombers_alive_count--;
                    out_message.type = (i == winner) ? BOMBER_WIN : BOMBER_DIE;

                    send_message(bomber_fds[i][1], &out_message);

                    omp out_message_print = {bomber_pids[i], &out_message};

                    print_output(NULL, &out_message_print, NULL, NULL);

                    int child_status;
                    waitpid(bomber_pids[i], &child_status, 0);

                    close(bomber_fds[i][0]);
                    close(bomber_fds[i][1]);

                    continue;
                }

                if (in_message.type == BOMBER_START)
                {    
                    out_message.type = BOMBER_LOCATION;
                    out_message.data.new_position = bombers[i].position;

                    send_message(bomber_fds[i][1], &out_message);
                }
                else if (in_message.type == BOMBER_MOVE)
                {
                    if (in_message.data.target_position.x >= 0 && in_message.data.target_position.x < map_width && in_message.data.target_position.y >= 0 && in_message.data.target_position.y < map_height)
                    {
                        if (!obstacle_map[in_message.data.target_position.y][in_message.data.target_position.x])
                        {
                            if ((abs(in_message.data.target_position.x - bombers[i].position.x) + abs(in_message.data.target_position.y - bombers[i].position.y)) == 1)
                            {
                                bomber_map[bombers[i].position.y][bombers[i].position.x] = 0;
                                bomber_map[in_message.data.target_position.y][in_message.data.target_position.x] = 1;

                                bombers[i].position = in_message.data.target_position;
                            }
                        }
                    }

                    out_message.type = BOMBER_LOCATION;
                    out_message.data.new_position = bombers[i].position;

                    send_message(bomber_fds[i][1], &out_message);

                    omp out_message_print = {bomber_pids[i], &out_message};

                    print_output(NULL, &out_message_print, NULL, NULL);
                }
                else if (in_message.type == BOMBER_PLANT)
                {
                    out_message.type = BOMBER_PLANT_RESULT;

                    if (!bomb_map[bombers[i].position.y][bombers[i].position.x])
                    {
                        bomb_count++;

                        bombs = realloc(bombs, bomb_count * sizeof(bd));
                        bomb_position = realloc(bomb_position, bomb_count * sizeof(bd));

                        bomb_fds = realloc(bomb_fds, bomb_count * 2 * sizeof(int));
                        PIPE(bomb_fds[bomb_count - 1]);

                        bombs[bomb_count - 1] = in_message.data.bomb_info;
                        bomb_position[bomb_count - 1] = bombers[i].position;
                        bombs_active[bomb_count - 1] = 1;
                        bombs_active_count++;

                        bomb_map[bombers[i].position.y][bombers[i].position.x] = 1;

                        out_message.data.planted = 1;
                    }
                    else
                    {
                        out_message.data.planted = 0;
                    }
                    
                    send_message(bomber_fds[i][1], &out_message);

                    omp out_message_print = {bomber_pids[i], &out_message};

                    print_output(NULL, &out_message_print, NULL, NULL);

                    if (bomb_pids[bomb_count - 1] = fork())
                    {
                        ;
                    }
                    else
                    {
                        dup2(bomber_fds[bomb_count - 1][0], 0);
                        close(bomber_fds[bomb_count - 1][0]);
                        dup2(bomber_fds[bomb_count - 1][1], 1);
                        close(bomber_fds[bomb_count - 1][1]);

                        char *arg1;
                        int arg1_len = snprintf(NULL, 0, "%ld", in_message.data.bomb_info.interval);
                        arg1 = malloc((arg1_len + 1) * sizeof(char));
                        snprintf(arg1, arg1_len + 1, "%ld", in_message.data.bomb_info.interval);

                        execl("./bomb", "./bomb", arg1, NULL);
                    }
                }
                else if (in_message.type == BOMBER_SEE)
                {
                    out_message.type = BOMBER_VISION;

                    unsigned int object_count = 0;
                    od objects[24];

                    // TOP: in order of decreasing i (according to array indexing)
                    for (unsigned int x = bombers[i].position.x, start_y = bombers[i].position.y - 1,  end_y = start_y - 3; start_y >= 0 && start_y > end_y; start_y--)
                    {
                        // Check for obtacles
                        if (obstacle_map[start_y][x])
                        {
                            objects[object_count++] = (od) {{x, start_y}, OBSTACLE};
                            break;
                        }

                        // Check for bombs
                        if (bomb_map[start_y][x])
                        {
                            objects[object_count++] = (od) {{x, start_y}, BOMB};
                        }

                        // Check for fellow bombers
                        if (bomber_map[start_y][x])
                        {
                            objects[object_count++] = (od) {{x, start_y}, BOMBER};
                        }
                    }

                    // DOWN: in order of increasing i (according to array indexing)
                    for (unsigned int x = bombers[i].position.x, start_y = bombers[i].position.y + 1,  end_y = start_y + 3; start_y < map_height && start_y < end_y; start_y++)
                    {
                        // Check for obtacles
                        if (obstacle_map[start_y][x])
                        {
                            objects[object_count++] = (od) {{x, start_y}, OBSTACLE};
                            break;
                        }

                        // Check for bombs
                        if (bomb_map[start_y][x])
                        {
                            objects[object_count++] = (od) {{x, start_y}, BOMB};
                        }

                        // Check for fellow bombers
                        if (bomber_map[start_y][x])
                        {
                            objects[object_count++] = (od) {{x, start_y}, BOMBER};
                        }
                    }

                    // RIGHT: in order of increasing j (according to array indexing)
                    for (unsigned int start_x = bombers[i].position.x + 1, y = bombers[i].position.y,  end_x = start_x + 3; start_x < map_width && start_x < end_x; start_x++)
                    {
                        // Check for obtacles
                        if (obstacle_map[y][start_x])
                        {
                            objects[object_count++] = (od) {{start_x, y}, OBSTACLE};
                            break;
                        }

                        // Check for bombs
                        if (bomb_map[y][start_x])
                        {
                            objects[object_count++] = (od) {{start_x, y}, BOMB};
                        }

                        // Check for fellow bombers
                        if (bomber_map[y][start_x])
                        {
                            objects[object_count++] = (od) {{start_x, y}, BOMBER};
                        }
                    }

                    // LEFT: in order of decreasing j (according to array indexing)
                    for (unsigned int start_x = bombers[i].position.x - 1, y = bombers[i].position.y,  end_x = start_x - 3; start_x >=0 && start_x > end_x; start_x--)
                    {
                        // Check for obtacles
                        if (obstacle_map[y][start_x])
                        {
                            objects[object_count++] = (od) {{start_x, y}, OBSTACLE};
                            break;
                        }

                        // Check for bombs
                        if (bomb_map[y][start_x])
                        {
                            objects[object_count++] = (od) {{start_x, y}, BOMB};
                        }

                        // Check for fellow bombers
                        if (bomber_map[y][start_x])
                        {
                            objects[object_count++] = (od) {{start_x, y}, BOMBER};
                        }
                    }

                    // Send message with object count
                    out_message.data.object_count = object_count;
                    send_message(bomber_fds[i][1], &out_message);

                    // Send message with object data
                    send_object_data(bomber_fds[i][1], object_count, objects);
                    
                    omp out_message_print = {bomber_pids[i], &out_message};

                    print_output(NULL, &out_message_print, NULL, objects);
                }
            }
        }

        // usleep(1000);
    }

}