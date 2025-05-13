## Overview
A Java-based command-line application that manages pet adoptions for an animal shelter. This system allows users to view available pets, process adoption requests, and track adoption status. The Pet Adoption System is designed to streamline the pet adoption process by providing a simple interface for both shelter staff and potential adopters. The system maintains records of available animals (cats and dogs), handles adoption requests, and tracks completed adoptions.

## Features
- View available pets for adoption (cats and dogs)
- Process new adoption requests
- Track adoption status
- Maintain records of completed adoptions
- Input validation and error handling
- Pet-specific information management
- Unique ID generation for adopters and animals

## Class Structure
- Adoptable: Interface defining adoption-related behaviors
- Animal: Abstract base class for all animals
- Cat: Concrete class extending Animal
- Dog: Concrete class extending Animal
- Adopter: Class representing potential pet adopters
- AdoptionRecord: Class maintaining adoption transaction records
- AdoptionRepository: Class managing completed adoptions
- AnimalRepository: Class managing available animals
- AdoptionException: Custom exception class for adoption-related errors
PetAdoptionSystem: Main class containing the application logic
