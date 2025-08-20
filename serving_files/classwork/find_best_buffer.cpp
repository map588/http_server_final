#include <chrono>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

using namespace std;
using namespace std::chrono;

struct BufferTestResult {
  size_t bufferSize;
  double duration;
};

struct testResults {
  string filename;
  vector<BufferTestResult> results;
};

class MappedFileBuffer {
private:
  const char *data;
  size_t size;
  int fd;

public:
  explicit MappedFileBuffer(const std::string &filename)
      : data(nullptr), size(0), fd(-1) {
    // Open the file as read only
    fd = open(filename.c_str(), O_RDONLY);
    if (fd == -1) {
      throw std::runtime_error("Error opening file.");
    }

    // Use stat to get file size
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
      close(fd);
      throw std::runtime_error("Error getting file size.");
    }
    size = sb.st_size;

    // If the size is 0, somethings wrong
    if (size == 0) {
      close(fd);
      throw std::runtime_error("File is empty.");
    }

    // Map the file into my processes address space, in read-only copy-on-write
    // behavior, without sharing the memory map. I only need one mapping,
    // because only my main process is accessing the file.
    void *mapped = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);

    if (mapped == MAP_FAILED) {
      close(fd);
      throw std::runtime_error("Error mapping file.");
    }

    // Mapped buffer now has a mapped buffer
    data = static_cast<const char *>(mapped);
  }
  // Delete copy constructor and assignment
  MappedFileBuffer(const MappedFileBuffer &) = delete;
  MappedFileBuffer &operator=(const MappedFileBuffer &) = delete;

  // Add move constructor and assignment if needed
  MappedFileBuffer(MappedFileBuffer &&other) noexcept
      : data(other.data), size(other.size), fd(other.fd) {
    other.data = nullptr;
    other.fd = -1;
  }

  ~MappedFileBuffer() {
    if (data) {
      munmap(const_cast<char *>(data), size);
    }
    if (fd != -1) {
      close(fd);
    }
  }

  const char *get_data() const { return data; }
  size_t get_size() const { return size; }
};

class WritableMemoryMappedFile {
private:
  char *data;
  size_t size;
  int fd;
  std::string filename;

public:
  explicit WritableMemoryMappedFile(const std::string &fname, size_t fileSize)
      : data(nullptr), size(fileSize), fd(-1), filename(fname) {

    // Create the file with write permissions
    fd = open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
      throw std::runtime_error("Error creating output file: " +
                               string(strerror(errno)));
    }

    // Set the file size by seeking and writing a byte
    if (lseek(fd, fileSize - 1, SEEK_SET) == -1) {
      close(fd);
      throw std::runtime_error("Error seeking in file: " +
                               string(strerror(errno)));
    }

    // Write a single byte at the end
    if (write(fd, "", 1) != 1) {
      close(fd);
      throw std::runtime_error("Error writing to file: " +
                               string(strerror(errno)));
    }

    // Map the file with write permissions
    void *mapped =
        mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED) {
      close(fd);
      throw std::runtime_error("Error mapping output file: " +
                               string(strerror(errno)));
    }

    data = static_cast<char *>(mapped);
  }

  ~WritableMemoryMappedFile() {
    if (data) {
      munmap(data, size);
    }
    if (fd != -1) {
      close(fd);
    }
    unlink(filename.c_str());
  }

  // Delete copy constructor and assignment
  WritableMemoryMappedFile(const WritableMemoryMappedFile &) = delete;
  WritableMemoryMappedFile &
  operator=(const WritableMemoryMappedFile &) = delete;

  // Move constructor
  WritableMemoryMappedFile(WritableMemoryMappedFile &&other) noexcept;

  char *get_data() { return data; }
  const char *get_data() const { return data; }
  size_t get_size() const { return size; }
};

testResults testBufferSize(const string &inputPath, const string &outputPath,
                           size_t bufferSize) {
  int iterations = 30;
  vector<BufferTestResult> results;

  // Get input file size first
  struct stat sb;
  if (stat(inputPath.c_str(), &sb) == -1) {
    throw std::runtime_error("Error getting input file size");
  }
  size_t fileSize = sb.st_size;

  for (int i = 0; i < iterations; i++) {
    BufferTestResult result;
    result.bufferSize = bufferSize;

    MappedFileBuffer inFile(inputPath);
    // Create new output file for each iteration
    WritableMemoryMappedFile outFile(outputPath + "." + to_string(i), fileSize);

    const char *src = inFile.get_data();
    char *dst = outFile.get_data();

    auto start = high_resolution_clock::now();

    // Copy in chunks of bufferSize
    size_t remaining = fileSize;
    size_t offset = 0;
    while (remaining > 0) {
      size_t chunk = std::min(bufferSize, remaining);
      memcpy(dst + offset, src + offset, chunk);
      offset += chunk;
      remaining -= chunk;
    }

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<nanoseconds>(stop - start);

    result.duration = duration.count() / 1000000.0;
    results.push_back(result);
    // outFile destructor will clean up the file when it goes out of scope
  }

  testResults testResults;
  testResults.filename = inputPath;
  testResults.results = results;

  return testResults;
}

void createFile(const string &filename, size_t fileSize) {
  // First create the file if it doesn't exist
  int fd = open(filename.c_str(), O_RDWR | O_CREAT, 0644);
  if (fd == -1) {
    std::cerr << "Failed to create file: " << filename << std::endl;
    std::cerr << "Errno: " << errno << " - " << strerror(errno) << std::endl;
    std::cerr << "Current working directory: ";
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
      std::cerr << cwd << std::endl;
    } else {
      std::cerr << "Unable to get current directory" << std::endl;
    }
    throw std::runtime_error("Error creating file " + filename + ": " +
                             strerror(errno));
  }

  // Print file descriptor and initial size
  struct stat sb;
  if (fstat(fd, &sb) == 0) {
    std::cout << "File created with fd: " << fd
              << " initial size: " << sb.st_size << std::endl;
  }

  close(fd); // Close it before truncating

  // Now truncate it to the right size
  if (truncate(filename.c_str(), fileSize) == -1) {
    std::cerr << "Failed to truncate file to size " << fileSize << std::endl;
    std::cerr << "Errno: " << errno << " - " << strerror(errno) << std::endl;
    throw std::runtime_error("Error setting file size: " +
                             string(strerror(errno)));
  }

  // Reopen with read/write permissions
  fd = open(filename.c_str(), O_RDWR);
  if (fd == -1) {
    std::cerr << "Failed to reopen file after truncate" << std::endl;
    std::cerr << "Errno: " << errno << " - " << strerror(errno) << std::endl;
    throw std::runtime_error("Error reopening file: " +
                             string(strerror(errno)));
  }

  // Map the file with write permissions
  void *mapped =
      mmap(nullptr, fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (mapped == MAP_FAILED) {
    std::cerr << "Failed to map file of size " << fileSize << std::endl;
    std::cerr << "Errno: " << errno << " - " << strerror(errno) << std::endl;
    close(fd);
    throw std::runtime_error("Error mapping file: " + string(strerror(errno)));
  }

  char *data = static_cast<char *>(mapped);

  // Generate random data
  minstd_rand rand_generator;
  random_device rand_device;
  rand_generator.seed(rand_device());
  uniform_int_distribution<int> distribution(0, 25);

  // Write directly to mapped memory
  for (size_t i = 0; i < fileSize; i++) {
    data[i] = distribution(rand_generator) + 65;
  }

  // Sync to ensure data is written
  if (msync(mapped, fileSize, MS_SYNC) == -1) {
    std::cerr << "Failed to sync mapped memory" << std::endl;
    std::cerr << "Errno: " << errno << " - " << strerror(errno) << std::endl;
  }

  // Cleanup
  munmap(mapped, fileSize);
  close(fd);
}

int main() {
  try {
    testResults results;
    double minTimeTaken[3] = {0, 0, 0};
    uint64_t blockSize[3] = {0, 0, 0};
    uint64_t testBlockSize = 0;
    int fileSize = 0;
    char filename[32]{0};
    char filename_copy[32]{0};
    map<int, int> ratio_weight;
    map<int, int> Buffer_weight;

    // Main testing loop remains the same until the results writing part
    for (int j = 1; j <= 4096; j *= 2) {
      fileSize = j * 4096 * 4;
      snprintf(filename, 32, "test_%dk.txt", fileSize / 1024);
      snprintf(filename_copy, 32, "test_%dk_copy.txt", fileSize / 1024);
      cout << "Creating test file: " << filename << " of size " << fileSize
           << " bytes" << endl;

      createFile(filename, fileSize);

      for (int i = 1; i <= 1024; i *= 2) {
        testBlockSize = i * 1024;
        results = testBufferSize(filename, filename_copy, testBlockSize);
        double averageTimeTaken = 0;

        for (auto result : results.results) {
          averageTimeTaken += result.duration;
        }
        averageTimeTaken /= results.results.size();

        if (averageTimeTaken < minTimeTaken[0] || minTimeTaken[0] == 0) {
          minTimeTaken[0] = averageTimeTaken;
          blockSize[0] = testBlockSize;
        } else if (averageTimeTaken < minTimeTaken[1] || minTimeTaken[1] == 0) {
          minTimeTaken[1] = averageTimeTaken;
          blockSize[1] = testBlockSize;
        } else if (averageTimeTaken < minTimeTaken[2] || minTimeTaken[2] == 0) {
          minTimeTaken[2] = averageTimeTaken;
          blockSize[2] = testBlockSize;
        }
      }
      unlink(filename);

      cout << filename << endl
           << blockSize[0] / 1024 << "k => " << minTimeTaken[0] << " ms" << endl
           << blockSize[1] / 1024 << "k => " << minTimeTaken[1] << " ms" << endl
           << blockSize[2] / 1024 << "k => " << minTimeTaken[2] << " ms" << endl
           << endl;

      double ratios[3] = {(double)fileSize / blockSize[0],
                          (double)fileSize / blockSize[1],
                          (double)fileSize / blockSize[2]};

      cout << "Size to best block ratio: " << endl
           << ratios[0] << endl
           << ratios[1] << endl
           << ratios[2] << endl
           << endl;
    }

    return 0;
  } catch (const std::exception &e) {
    cerr << e.what() << std::endl;
    return 1;
  }
}
