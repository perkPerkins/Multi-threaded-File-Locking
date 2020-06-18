#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <fcntl.h>
#include <sys/types.h>
#include <cerrno>
#include <fstream>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <algorithm>

using namespace std;

pthread_mutex_t myMutex;
bool file_is_sorted = false;
int list_len;

bool validate_user_input(string input) {
    if(input.at(0) == '-') {
        cout << "No negative integers allowed as input. Try again.\n";
        return false;
    }
    for(char i : input) {
        if(!isdigit(i)) {
            cout << "The argument you supplied was not an integer. Please try again.\n";
            return false;
        }
    }
    return true;
}

void populate_file(ofstream& file, int nums){
    srandom((unsigned) time(nullptr));
    for(int i = 0; i < nums; i++) file << random() % 100 << "\t";
}

int calculate_lock_length(int start, int end) {
    int lock_size = 0, count = 0;
    fstream file;
    string file_name = "input_file.txt", element;
    file.open(file_name.c_str());

    while(file >> element) {
        if(count >= start and count <= end) {
            for(int i = 0; i < element.size(); i++) {
                lock_size++; //add 1 byte to buffer for each digit of each number before starting index
            }
	    lock_size++; // 1 byte for each '\t' character
        }
        count++;
    }
    file.close();
    return lock_size;
}

int calculate_buffer(int start) {
    int buffer = 0, count = 0;
    fstream file;
    string file_name = "input_file.txt", element;
    file.open(file_name.c_str());

    while(count < start) {
        file >> element;
        for(int i = 0; i < element.size(); i++) {
            buffer++; //add 1 byte to buffer for each digit of each number before starting index
        }
        buffer ++;    // 1 byte for each '\t' character
        count++;
    }
    file.close();
    return buffer;
}

void swap(int *xp, int *yp)
{
    int temp = *xp;
    *xp = *yp;
    *yp = temp;
}

// A function to implement bubble sort, courtesy of GeeksForGeeks: https://www.geeksforgeeks.org/bubble-sort/
void bubbleSort(int arr[], int n)
{
    int i, j;
    for (i = 0; i < n-1; i++)

        // Last i elements are already in place
        for (j = 0; j < n-i-1; j++)
            if (arr[j] > arr[j+1])
                swap(&arr[j], &arr[j+1]);
}
// A function to implement selection sort, courtesy of GeeksForGeeks: https://www.geeksforgeeks.org/selection-sort/
void selectionSort(int arr[], int n)
{
    int i, j, min_idx;

    // One by one move boundary of unsorted subarray
    for (i = 0; i < n-1; i++)
    {
        // Find the minimum element in unsorted array
        min_idx = i;
        for (j = i+1; j < n; j++)
            if (arr[j] < arr[min_idx])
                min_idx = j;

        // Swap the found minimum element with the first element
        swap(&arr[min_idx], &arr[i]);
    }
}

void * check_array(void * pParam) {
    while(!file_is_sorted) {
        fstream file;
        string file_name = "input_file.txt", element;
        file.open(file_name.c_str());
        int counter = 0, num1, num2;

        while(file >> element) {
	    if(counter == 0) {
	        num1 = stoi(element);
	        counter++;
		continue;
	    }
	    num2 = stoi(element);
	    if(num2 < num1) {
	        break;
	    }
	    num1 = num2;
	    counter++;
	    if(counter == list_len) file_is_sorted = true;
        }
	file.close();    
    }
}

void * thread_sort(void * pParam) {
    while(!file_is_sorted) {
    int fd = open("input_file.txt", O_RDWR);
    int i = (rand() % list_len), j = (rand() % list_len);

    if(j < i) {
        int temp = i;
        i = j;
        j = temp;
    }

    int start_buffer = calculate_buffer(i);
    int lock_length = calculate_lock_length(i, j);
    int threadID = (int)(long)pParam;

    struct flock fl = {F_UNLCK, SEEK_SET, start_buffer, lock_length, 0};

    fl.l_type = F_WRLCK; //exclusive lock
    if(fcntl(fd, F_OFD_SETLKW, &fl) == -1) {
        perror("Cant set exclusive lock on this section");
        exit(1);
    }
    else if(fl.l_type != F_UNLCK) {
	pthread_mutex_lock(&myMutex);
	printf("Elements %d - %d of the file have been locked by thread\t%d\n", i, j, threadID);
        ifstream file("input_file.txt");
        string element;
        int list_range_count = 0, index = 0;
	int element_array[list_len];
  	int sub_section_array[(j-i) + 1];

        while(list_range_count < list_len) {
            file >> element;
            if(list_range_count < i or list_range_count > j) {
                element_array[list_range_count] = stoi(element);
                list_range_count++;
            }
            else if(list_range_count < j + 1) {
                sub_section_array[index] = stoi(element);
                element_array[list_range_count] = stoi(element);
                index++;
                list_range_count++;
            }
        }

        int random_algorithm = random() % 3;
        int array_size = sizeof(sub_section_array) / sizeof(sub_section_array[0]);
        printf("Elements in range before sorting: ");
        for(int x = 0; x < array_size; x++) {
            printf("%d\t", sub_section_array[x]);
        }

        switch(random_algorithm) {
            case 0:
                sort(sub_section_array, sub_section_array + array_size);
                break;
            case 1:
                bubbleSort(sub_section_array, array_size);
                break;
            case 2:
                selectionSort(sub_section_array, array_size);
                break;
            default:
                break;
        }
        cout << "\nElements in range  after sorting: "; // sorting type has been removed to improve readability
        for (int x = 0; x < array_size; x++) {
            printf("%d\t", sub_section_array[x]);
        }

        fstream write_back_to_file;
        write_back_to_file.open("input_file.txt");
        list_range_count = 0;
        for(int x = 0; x < list_len; x++) {
            if(x < i or x > j) {
                write_back_to_file << to_string(element_array[x]) << "\t";
            }
            else{
                write_back_to_file << to_string(sub_section_array[list_range_count]) << "\t";
                list_range_count++;
            }
        }

        write_back_to_file.close();

        cout << "\nList elements before sorting: ";
        for(int x = 0; x < list_len; x++) {
            printf("%d\t", element_array[x]);
        }

        // just prints out the contents of the file
        ifstream stream("input_file.txt");
        string line;
        cout << "\nList elements  after sorting: ";
        while(getline(stream, line)) {
            cout << line;
        }
        printf("\nElements %d - %d of the file have been unlocked by thread\t%d\n", i, j, threadID);

        cout << "\n\n";
        stream.close();
    
        pthread_mutex_unlock(&myMutex);
    }
    else{
        cout << "File segment is unlocked\n";
    }

    //getchar();
    fl.l_type = F_UNLCK;
    fcntl(fd, F_OFD_SETLKW, &fl);
    }
    return (void*) nullptr;
}

int main(int argc, const char* argv[]) {
    //error handling
    if(argc == 1) {
        cout << "Please provide the number of elements you want written to a file (I will square this number and create that many elements).\n";
        exit(1);
    }
    else if(argc > 2) {
        cout << "Too many arguments\n";
        exit(1);
    }

    //populate file
    ofstream input_file("input_file.txt");
    if(!validate_user_input(argv[1])) exit(1);

    list_len = stoi(argv[1]) * stoi(argv[1]);
    populate_file(input_file, list_len);
    input_file.close();

    string user_input;
    cout << "How many threads should be created?";
    cin >> user_input;
    while(!validate_user_input(user_input)) {
        cin >> user_input;
    }

    pthread_t threads[stoi(user_input)];
    pthread_t checker_thread;
    pthread_mutex_init(&myMutex, nullptr);

    pthread_create(&checker_thread, nullptr, check_array, (void*)(long)1);
    for(int i = 0; i < stoi(user_input); i++) {
        pthread_create(&threads[i], nullptr, thread_sort, (void*)(long)i);
    }

    pthread_join(checker_thread, nullptr);
    for(int i = 0; i < stoi(user_input); i++) {
        pthread_join(threads[i], nullptr);
    }
    
    cout << "For the life of me I can't figure out why the spacing is so weird, but it works! File has been sorted!\n";
    return 0;
}

