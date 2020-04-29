import sys
from math import ceil
from nltk import tokenize, download

def print_paragraph_entry(sentences, prgnum):
    paragraph = sentences[prgnum * 4:4 * (prgnum + 1)]

    for sentence in paragraph:
        print(sentence)

def wait_for_user_request(textfile):
    full_text = textfile.read()
    sentences = tokenize.sent_tokenize(full_text)

    for line in sys.stdin:
        command = line.strip().split(' ')
        command_name = command[0]

        if command_name == "exit":
            break

        if command_name == "getprg":
            prgnum = int(command[1])

            print_paragraph_entry(sentences, prgnum)
        elif command_name == "getnumprgs":
            print(ceil(len(sentences) / 4))

def main():
    if len(sys.argv) != 2:
        print("Usage: %s textfile" % (sys.argv[0]))
        sys.exit(1)

    download('punkt')

    with open(sys.argv[1]) as textfile:
        wait_for_user_request(textfile)


if __name__ == '__main__':
    main()
