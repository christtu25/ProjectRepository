package adoptionsystemproject; // Declaring class package

public class AdoptionException extends Exception // Declaring AdoptionException class extending Exception
{
    private static final long serialVersionUID = 1L; // Declaring serializable operations to class for deserialization

    public AdoptionException(String message) // Declaring message attribute variable to class operations
    {
        super(message); // Declaring super operation to message variable
    }

    // No need to override getMessage() unless specific behavior is needed
}
