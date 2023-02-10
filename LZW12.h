#pragma once
#include <cstring>
#include <fstream>
#include <memory>
#include "BitIO.h"

#define BITS 12
#define MAX_CODE ((1 << BITS) - 1)
#define TABLE_SIZE 5021
#define END_OF_STREAM 256
#define FIRST_CODE 257
#define UNUSED -1

struct dictionary {
	int parentNode{};
	int codeValue{};
	char character{};
};

dictionary dict[TABLE_SIZE];

char decodeStack[TABLE_SIZE];

unsigned int hashChildNode(int parentNode, int childCharacter) {
	int index{}, offset{};
	index = (childCharacter << (BITS - 8)) ^ parentNode;
	if (index == 0)
		offset = 1;
	else
		offset = TABLE_SIZE - index;
	for (;;) {
		if (dict[index].codeValue == UNUSED)
			return index;
		if (dict[index].parentNode == parentNode && dict[index].character == (char)childCharacter)
			return index;
		index -= offset;
		if (index < 0)
			index += TABLE_SIZE;
	}
}

void compressFile(std::fstream& input, std::unique_ptr<stl::BitFile>& output) {
	int nextCode{}, character{}, stringCode{};
	unsigned int index{}, i{};
	nextCode = FIRST_CODE;
	for (i = 0; i < TABLE_SIZE; ++i)
		dict[i].codeValue = UNUSED;
	if (stringCode = input.get(); stringCode == EOF)
		stringCode = END_OF_STREAM;
	while ((character = input.get()) != EOF) {
		index = hashChildNode(stringCode, character);
		if (dict[index].codeValue != UNUSED)
			stringCode = dict[index].codeValue;
		else {
			if (nextCode <= MAX_CODE) {
				dict[index].parentNode = stringCode;
				dict[index].codeValue = nextCode++;
				dict[index].character = (char)character;
			}
			stl::outputBits(output, (std::uint32_t)stringCode, BITS);
			stringCode = character;
		}
	}
	stl::outputBits(output, (std::uint32_t)stringCode, BITS);
	stl::outputBits(output, (std::uint32_t)END_OF_STREAM, BITS);
}

//this routine decodes a string from the dictionary and stores it in the decodeStack data structure.
//it returns a count to the calling program of how many characters were placed in the stack
unsigned int decodeString(unsigned int count, unsigned int code) {
	while (code > 255) {
		decodeStack[count++] = dict[code].character;
		code = dict[code].parentNode;
	}
	decodeStack[count++] = (char)code;
	return count;
}


void expandFile(std::unique_ptr<stl::BitFile>& input, std::fstream& output) {
	unsigned int nextCode{}, newCode{}, oldCode{};
	int character{}, count{};
	nextCode = FIRST_CODE;
	oldCode = (unsigned int)stl::inputBits(input, BITS);
	if (oldCode == END_OF_STREAM)
		return;
	character = oldCode;
	output.put(character);
	while ((newCode = stl::inputBits(input, BITS)) != END_OF_STREAM) {
		if (newCode >= nextCode) { //test for the incomplete dictionary exception
			decodeStack[0] = (char)character;
			count = decodeString(1, oldCode);
		}
		else
			count = decodeString(0, newCode);
		count--;
		character = decodeStack[count]; //isolate the first character
		for (int i = count; i > -1; --i){
			output.put(decodeStack[i]);
		}
			
		if (nextCode <= MAX_CODE) {//insert new phrase(oldCode + character) in dictionary
			dict[nextCode].parentNode = oldCode;
			dict[nextCode].character = (char)character;
			nextCode++;
		}
		oldCode = newCode;
	}
}