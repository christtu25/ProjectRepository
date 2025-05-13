package adoptionsystemproject; // Declaring class package

import java.util.List; // Declaring class list operations
import java.util.Scanner; // Declaring class variable operations
import java.util.stream.Collectors; // Declaring class streaming operations

public class PetAdoptionSystem // Declaring PetAdoptionSystem class operations
{
    private static final Scanner scanner = new Scanner(System.in); // Declaring private static final scanner initialization
    private static final AnimalRepository animalRepository = new AnimalRepository(); // Declaring private static final AnimalRepository initialization
    private static final AdoptionRepository adoptionRepository = new AdoptionRepository(); // Declaring private static final AdoptionRepository initialization

    public static void main(String[] args) throws AdoptionException // Declaring class main operations
    {
    	initializeAnimals(); // Initialize with one cat and one dog
    	
        while (true) // Declaring while loop when true
        {
            System.out.println("Welcome to the Pet Adoption System!"); // Printing welcome prompt to user
            System.out.println("1. Check Adoption Status"); // Printing user prompt of option 1
            System.out.println("2. Prepare an Adoption Request"); // Printing user prompt of option 2
            System.out.println("Please choose an option (or any other key to exit):"); // Printing user prompt for option output

            String option = scanner.nextLine(); // Declaring new line to scanner option 

            switch (option) // Declaring switch operation by option variable
            {
                case "1": // Declaring case 1
                    checkAdoptionStatus(); // ... case 1 = AdoptionStatus
                    break; // Declaring break to case 1
                case "2": // Declaring case 2 
                    prepareAdoptionRequest(); // ... case 2 = AdoptionRequest 
                    break; // Declaring break to case 2 
                default: // Declaring default case
                    System.out.println("Exiting system."); // Printing exit prompt to case 2
                    return; // Declaring return instruction after case operation
            }
        }
    }
    
    private static void initializeAnimals() // Declaring available animals in static void repository 
    {
        Cat cat1 = new Cat("cat01", "Whiskers", 3, "Female", true); // Declaring 1st cat
        Cat cat2 = new Cat("cat02", "Shadow", 4, "Male", false); // Declaring 2nd cat
        Cat cat3 = new Cat("cat03", "Luna", 2, "Female", true); // Declaring 3rd cat
        Cat cat4 = new Cat("cat04", "Oliver", 1, "Male", false); // Declaring 4th cat
        Cat cat5 = new Cat("cat05", "Milo", 3, "Male", true); // Declaring 5th cat
        
        Dog dog1 = new Dog("dog01", "Buddy", 5, "Male", "Labrador"); // Declaring 1st dog
        Dog dog2 = new Dog("dog02", "Max", 6, "Male", "Beagle"); // Declaring 2nd dog
        Dog dog3 = new Dog("dog03", "Bella", 2, "Female", "German Shepherd"); // Declaring 3rd dog
        Dog dog4 = new Dog("dog04", "Charlie", 4, "Male", "Bulldog"); // Declaring 4th dog
        Dog dog5 = new Dog("dog05", "Daisy", 3, "Female", "Poodle"); // Declaring 5th dog
        
        animalRepository.addAnimal(cat1); // Declaring 1st cat into repository list
        animalRepository.addAnimal(cat2); // Declaring 2nd cat into repository list
        animalRepository.addAnimal(cat3); // Declaring 3rd cat into repository list
        animalRepository.addAnimal(cat4); // Declaring 4th cat into repository list
        animalRepository.addAnimal(cat5); // Declaring 5th cat into repository list
        
        animalRepository.addAnimal(dog1); // Declaring 1st dog into repository list
        animalRepository.addAnimal(dog2); // Declaring 2nd dog into repository list
        animalRepository.addAnimal(dog3); // Declaring 3rd dog into repository list
        animalRepository.addAnimal(dog4); // Declaring 4th dog into repository list
        animalRepository.addAnimal(dog5); // Declaring 5th dog into repository list
    }
    
    private static void checkAdoptionStatus() // Declaring repository status prompt 
    {
        List<AdoptionRecord> completedAdoptions = adoptionRepository.getCompletedAdoptions(); // Declaring adopters list
        
        if (completedAdoptions.isEmpty()) // Declaring if statement when list is empty 
        {
            System.out.println("No adoptions have been completed yet."); // Printing user prompt for empty list
        } 
        
        else // Declaring else statement 
        {
            for (AdoptionRecord record : completedAdoptions) // Declaring for statement with completed adoptions list                    
            {
                System.out.println("Adopter: " + record.getAdopter().getName() + " has adopted " + record.getAnimal().getName() + "."); // Printing adoption list
            }
        }
    }

    private static void prepareAdoptionRequest() throws AdoptionException// Declaring prepareAdoptionRequest as private static void  
    {
        System.out.println("Please enter your name:"); // Printing for user name
        String name = scanner.nextLine(); // Declaring user option scanner operation to next line
        System.out.println("Please enter your age:"); // Printing for user age
        int age = Integer.parseInt(scanner.nextLine()); // Declaring user option scanner operation to next line
        System.out.println("Are you interested in adopting a Cat or a Dog?"); // Printing for user pet choice
        String animalType = scanner.nextLine(); // Declaring user option scanner operation to next line
        System.out.println("What is your preferred pet (optional, press Enter to skip):"); // Printing for adopter's preferred available option
        String preferredPet = scanner.nextLine(); // Declaring user option scanner operation to next line

        List<Animal> availableAnimals = animalRepository.getAvailableAnimals().stream() // Declaring list from animal repository through stream operation
                .filter(a -> animalType.equalsIgnoreCase("Cat") ? a instanceof Cat : a instanceof Dog) // Declaring filter instruction to a variable lambda to equals ignore case by Cat/Dog
                .collect(Collectors.toList()); // Declaring collect instruction for adoption list
        
        if (availableAnimals.isEmpty()) // Declaring if statement to empty available animal list 
        {
            System.out.println("Sorry, there are no available " + animalType + "s for adoption at this time."); // Printing no pet available prompt
            return; // Declaring return instruction to loop statement
        }

        System.out.println("Available " + animalType + "s:"); // Printing pet availability prompt
        for (Animal animal : availableAnimals) // Declaring for statement to availableAnimals from list 
        {
            System.out.println(animal.getDetails()); // Printing animal details from list
        }

        System.out.println("Enter the ID of the " + animalType + " you wish to adopt:"); // Printing user prompt for animal adoption type 
        String animalID = scanner.nextLine(); // Declaring scanner operation to new line by animalID variable
        Animal animal = animalRepository.findAnimalById(animalID);
        
        if (animal == null) // Declaring if statement for unavailable animal
        {
            System.out.println("Sorry, the selected " + animalType + " is not available."); // Printing user prompt for unavailability
            return; // Declaring return function to if statement
        }
        
        String adopterID = name.replaceAll(" ", "") + age; // makeshift unique ID
        Adopter adopter = new Adopter(adopterID, name, age, preferredPet); // Declaring adopter's variables
        try // Declaring try block for adoption request
        {
            adopter.requestAdoption(animal); // Declaring adopter to animal requested
            adoptionRepository.addAdoption(adopter, animal); // Declaring animal to adopter relationship
            System.out.println("Thank you for adopting a " + animalType + "!"); // Printing successful user prompt on request
        } 
        catch (AdoptionException e)  // Declaring catch block for exception
        {
            System.out.println(e.getMessage()); // Printing exception message
        }
    }
}
