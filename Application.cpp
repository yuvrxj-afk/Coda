﻿#include "Application.h"




namespace Coda {
	int Application::run(int argc, char** argv) {
		int result = EXIT_SUCCESS;
		try {
			return runnable(argc, argv);
		}
		catch (const std::exception& e) {
			std::cerr << "[CODA] A fatal error occurred in the interpreter.\n" << e.what() << std::endl;
			result = EXIT_FAILURE;
		}
		catch (...) {
			std::cerr << "[CODA] Unknown error occurred while executing the program." << std::endl;
			result = EXIT_FAILURE;
		}
		return EXIT_FAILURE;
	}

	int Application::runnable(int argc, char** argv)
	{
		Coda::Utils::ArgParser argParser = Coda::Utils::ArgParser();
		argParser.parse(argc, argv);

		if (argParser.getFlag("-h"))
			printHelpMessage();

#if _DEBUG
		mMainFileName = "Test/debug.coda";
#else 
		mMainFileName = argParser.getStandaloneValueAt(0);
#endif

		if (mMainFileName.empty()) {
			return repl();
		}

		Frontend::Importer importer = Frontend::Importer();
		std::string source = importer.import(mMainFileName);

		Runtime::Environment env = Runtime::Environment::root();
		if (source.empty()) {
			std::cout << "Source file is empty.\nOpening REPL...\n+------------------------+" << std::endl;
			return repl();
		}
		execute(source, env);

		if (argParser.getFlag("-w")) {
			std::cout << std::endl << "Press any key to continue...";
			std::cin.get();
		}

		return EXIT_SUCCESS;
	}


	int Application::repl() {
		std::cout << title << std::endl;
		std::cout << "Welcome to Coda REPL!" << std::endl;
		std::cout << "Type :q to exit" << std::endl;

		Runtime::Environment env = Runtime::Environment::root();
		while (1) {
			Coda::Error::Manager::reset();
			std::string source;
			std::cout << ">> ";
			std::getline(std::cin, source);
			if (source == ":q" || source == "exit" || source == "quit" || source == "e" || source == "q") break;

			execute(source, env);
		}
		return 0;
	}


	int Application::execute(std::string source, Coda::Runtime::Environment& env) {
		int result = EXIT_SUCCESS;
		if (source.empty()) {
			result = EXIT_FAILURE;
			std::cerr << "Empty source file.";
			return result;
		}

		std::vector<Coda::Frontend::Token> tokens = tokenize(source);

		IF_ERROR_RETURN(1);
		Coda::Frontend::Program program = parse(tokens);

		IF_ERROR_RETURN(1);
		interpret(program, env);


		return result;
	}

	void Application::printHelpMessage() {
		std::cout << title << std::endl;
		std::cout << helpMessage << std::endl;
	}


	std::vector<Coda::Frontend::Token> Application::tokenize(std::string source) {
		IF_ERROR_RETURN(std::vector<Coda::Frontend::Token>());
		Coda::Frontend::Lexer lexer = Coda::Frontend::Lexer();
		return lexer.tokenize(source);
	}

	Frontend::Program Application::parse(std::vector<Coda::Frontend::Token>& tokens) {
		IF_ERROR_RETURN(Frontend::Program());
		Coda::Frontend::Parser parser = Coda::Frontend::Parser(mMainFileName);
		return parser.parse(tokens);
	}


	Runtime::ValuePtr Application::interpret(Frontend::Program& program, Runtime::Environment& env) {
		IF_ERROR_RETURN_VALUE_PTR;
		Coda::Runtime::Interpreter inter = Coda::Runtime::Interpreter();
		return inter.evaluateProgram(program, env);
	}
}