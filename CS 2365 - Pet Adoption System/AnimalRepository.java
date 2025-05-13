package adoptionsystemproject; // Declaring class package

import java.util.ArrayList; // Declaring class array operations
import java.util.List; // Declaring class list operations
import java.util.stream.Collectors; // Declaring class streaming operations

class AnimalRepository // Declaring AnimalRepository class
{
    private List<Animal> animals = new ArrayList<>(); // Declaring animal as list variable for array operations

    public void addAnimal(Animal animal) // Declaring addAnimal system to operations
    {
        animals.add(animal); // Declaring animals to add operations
    }

    public List<Animal> getAllAnimals() // Declaring array from all animals in repository
    {
        return new ArrayList<>(animals); // Declaring animal list to return
    }

    public List<Animal> getAvailableAnimals() // Declaring to show array list
    {
        return animals.stream().filter(a -> !a.AdoptionStatus()).collect(Collectors.toList()); // Declaring stream operation to collect animal list from array to return
    }

    public Animal findAnimalById(String id) // Declaring animal ID search operations
    {
        return animals.stream().filter(a -> a.getAnimal_ID().equals(id)).findFirst().orElse(null); // Declaring filter operations to search unless list is empty 
    }
}
