#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <regex>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <fcntl.h>
#include <unistd.h>

using namespace std;

struct pid_info
{
    pid_t pid;
    string cmdline;
    string user;
    string path;
};

string tail(string source, size_t length)
{
    if (length >= source.size())
        return source;
    return source.substr(source.size() - length);
}

void print_result(struct pid_info info, string FD, string TYPE, string inode, string file, string option, string arg)
{
    bool printLine = false;
    // filter file infomation
    if (option == "default")
    {
        printLine = true;
    }
    else if (option == "c")
    {
        regex re(".*(" + arg + ").*");
        if (regex_match(info.cmdline, re))
            printLine = true;
    }
    else if (option == "t")
    {
        if (TYPE == arg)
            printLine = true;
    }
    else if (option == "f")
    {
        regex re(".*(" + arg + ").*");
        if (regex_match(file, re))
            printLine = true;
    }

    // print the infomation of the opened file
    if (printLine)
    {
        printf("%-9s %5d %10s %4s %9s %10s %s\n", info.cmdline.c_str(), info.pid, info.user.c_str(),
               FD.c_str(), TYPE.c_str(), inode.c_str(), file.c_str());
    }
}

void print_type(string type, struct pid_info info, string option, string arg)
{
    struct stat typestat;
    static ssize_t link_dest_size;
    static char link_dest[PATH_MAX];
    string TYPE;
    string inode;
    // Get the TYPE and NODE by calling stat on the proc/pid/type directory.
    if (!stat((info.path + type).c_str(), &typestat))
    {
        // Distinguish the type of file.
        if (S_ISDIR(typestat.st_mode))
            TYPE = "DIR";
        else if (S_ISREG(typestat.st_mode))
            TYPE = "REG";
        else if (S_ISCHR(typestat.st_mode))
            TYPE = "CHR";
        else if (S_ISFIFO(typestat.st_mode))
            TYPE = "FIFO";
        else if (S_ISSOCK(typestat.st_mode))
            TYPE = "SOCK";
        else
            TYPE = "unknown";
        // Get the file's inode number.
        inode = to_string(typestat.st_ino);
    }
    else
    {
        TYPE = "unknown";
        inode = "";
    }

    // Read the contents of symbolic link PATH.
    if ((link_dest_size = readlink((info.path + type).c_str(), link_dest, PATH_MAX - 1)) < 0)
    {
        if (errno == ENOENT)
            return;
        snprintf(link_dest, PATH_MAX, "%s (readlink: %s)", (info.path + type).c_str(), strerror(errno));
    }
    else
    {
        link_dest[link_dest_size] = '\0';
    }

    // Check the mode of file descriptor if needed.
    if (type.substr(0, 3) == "fd/")
    {
        // Distinguish the type of file descriptor.
        if ((typestat.st_mode & 0600) == 0600)
            type = type.substr(3) + 'u';
        else if ((typestat.st_mode & 0600) == S_IRUSR)
            type = type.substr(3) + 'r';
        else if ((typestat.st_mode & 0600) == S_IWUSR)
            type = type.substr(3) + 'w';

        if (tail(link_dest, 9) == "(deleted)")
            type = "del";
    }
    print_result(info, type, TYPE, inode, link_dest, option, arg);
}

void print_maps(struct pid_info info, string option, string arg)
{
    ifstream maps(info.path + "maps");
    stringstream ss;
    string mapsline;
    size_t offset;
    string device, file;
    ino_t inode;
    while (getline(maps, mapsline))
    {
        ss << mapsline;
        ss >> file >> file >> offset >> device >> inode >> file;
        ss.str("");
        ss.clear();
        if (device == "00:00" || inode == 0 || offset != 0)
            continue;
        
        print_result(info, "mem", "REG", to_string(inode), file, option, arg);
        /* printf("%-9s %5d %10s %4s %9s %10ld %s\n", info.cmdline.c_str(), info.pid, info.user.c_str(),
               "mem", "REG", inode, file.c_str()); */
    }
    maps.close();
}

void print_fds(struct pid_info info, string option, string arg)
{
    DIR *dir = opendir((info.path + "fd").c_str());
    if (dir == NULL)
    {
        char msg[1024];
        snprintf(msg, 1024, "%s (opendir: %s)", (info.path + "fd").c_str(), strerror(errno));

        print_result(info, "NOFD", "", "", msg, option, arg);
        /* printf("%-9s %5d %10s %4s %9s %10s %s\n", info.cmdline.c_str(), info.pid, info.user.c_str(),
               "NOFD", "", "", msg); */
        return;
    }
    struct dirent *de;
    while ((de = readdir(dir)))
    {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;
        string fdir = "fd/";
        fdir += de->d_name;
        print_type(fdir, info, option, arg);
    }
    closedir(dir);
}

void lsof_info(pid_t pid, string option, string arg)
{
    struct pid_info info;
    struct stat pidstat;
    struct passwd *pw;

    info.pid = pid;
    info.path = "/proc/" + to_string(pid) + '/';

    // Get the UID by calling stat on the proc/pid directory.
    if (!stat(info.path.c_str(), &pidstat))
    {
        pw = getpwuid(pidstat.st_uid);
        if (pw)
            info.user = pw->pw_name;
        else
            info.user = to_string(pidstat.st_uid);
    }
    else
    {
        info.user = "???";
    }

    // Read the command information in the stat file
    string cmd;
    ifstream statFile(info.path + "stat");
    if (!statFile)
    {
        cerr << "Couldn't read stat file!" << endl;
        return;
    }
    //getline(cmdlineFile, cmdline);
    statFile >> cmd >> cmd;
    statFile.close();
    info.cmdline = cmd.substr(1, cmd.length() - 2);

    // print opened files
    print_type("cwd", info, option, arg);
    print_type("root", info, option, arg);
    print_type("exe", info, option, arg);
    print_maps(info, option, arg);
    print_fds(info, option, arg);
}

int main(int argc, char *argv[])
{
    string option = "default";
    string arg;
    pid_t pid = 0;
    char *endptr;
    DIR *dir = opendir("/proc");

    if (argc == 3)
    {
        if (!strcmp(argv[1], "-c") || !strcmp(argv[1], "-f"))
        {
            option = argv[1][1];
            arg = argv[2];
        }
        else if (!strcmp(argv[1], "-t"))
        {
            option = 't';
            if (strcmp(argv[2], "REG") && strcmp(argv[2], "CHR") && strcmp(argv[2], "DIR") && strcmp(argv[2], "FIFO") && strcmp(argv[2], "SOCK") && strcmp(argv[2], "unknown"))
            {
                cerr << "Invalid TYPE option." << endl;
                return 0;
            }
            else
            {
                arg = argv[2];
            }
        }
    }

    printf("%-9s %5s %10s %4s %9s %10s %s\n",
           "COMMAND",
           "PID",
           "USER",
           "FD",
           "TYPE",
           "NODE",
           "NAME");

    struct dirent *de;
    while ((de = readdir(dir)))
    {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;
        // Only inspect directories that are PID numbers
        pid = strtol(de->d_name, &endptr, 10);
        if (*endptr != '\0')
            continue;
        lsof_info(pid, option, arg);
    }
    closedir(dir);
}